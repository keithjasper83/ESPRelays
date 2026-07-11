/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "OtaUpdateManager.h"

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "AppConfig.h"

namespace
{
    constexpr int MAX_HTTP_REDIRECTS = 5;

    bool isRedirectStatus(int statusCode)
    {
        return statusCode == HTTP_CODE_MOVED_PERMANENTLY ||
               statusCode == HTTP_CODE_FOUND ||
               statusCode == HTTP_CODE_SEE_OTHER ||
               statusCode == HTTP_CODE_TEMPORARY_REDIRECT ||
               statusCode == 308;
    }

    bool resolveFinalDownloadUrl(const String &initialUrl, String &resolvedUrl, String &error)
    {
        String currentUrl = initialUrl;

        for (int i = 0; i < MAX_HTTP_REDIRECTS; i++)
        {
            WiFiClientSecure client;
            client.setInsecure();

            HTTPClient http;
            if (!http.begin(client, currentUrl))
            {
                error = "Failed to initialize HTTPS client for redirect resolution";
                return false;
            }

            http.setUserAgent("esp32-relay-ota-update");
            http.addHeader("Accept", "application/octet-stream");
            const char *headers[] = {"Location"};
            http.collectHeaders(headers, 1);

            int statusCode = http.sendRequest("HEAD");
            if (statusCode == HTTP_CODE_METHOD_NOT_ALLOWED)
            {
                statusCode = http.GET();
            }

            if (statusCode <= 0)
            {
                error = "Failed to resolve OTA redirect: HTTP " + String(statusCode);
                http.end();
                return false;
            }

            if (isRedirectStatus(statusCode))
            {
                const String nextUrl = http.header("Location");
                http.end();

                if (nextUrl.length() == 0)
                {
                    error = "Redirect response missing Location header";
                    return false;
                }

                currentUrl = nextUrl;
                continue;
            }

            if (statusCode >= 200 && statusCode < 300)
            {
                http.end();
                resolvedUrl = currentUrl;
                return true;
            }

            error = "Unexpected HTTP status while resolving OTA URL: " + String(statusCode);
            http.end();
            return false;
        }

        error = "Too many HTTP redirects while resolving OTA URL";
        return false;
    }

    String extractJsonString(const String &json, const char *key, int fromIndex = 0)
    {
        const String pattern = String("\"") + key + "\"";
        const int keyIndex = json.indexOf(pattern, fromIndex);
        if (keyIndex < 0)
        {
            return String();
        }

        const int colonIndex = json.indexOf(':', keyIndex + static_cast<int>(pattern.length()));
        if (colonIndex < 0)
        {
            return String();
        }

        const int firstQuote = json.indexOf('"', colonIndex + 1);
        if (firstQuote < 0)
        {
            return String();
        }

        int cursor = firstQuote + 1;
        String value;
        value.reserve(64);

        while (cursor < json.length())
        {
            const char c = json[cursor];
            if (c == '\\' && cursor + 1 < json.length())
            {
                value += json[cursor + 1];
                cursor += 2;
                continue;
            }

            if (c == '"')
            {
                return value;
            }

            value += c;
            cursor++;
        }

        return String();
    }
}

void OtaUpdateManager::begin()
{
    lastCheckOkFlag = false;
    updateAvailableFlag = false;
    latestVersionValue = "";
    selectedAssetUrlValue = "";
    lastMessageValue = "No OTA check yet";
}

bool OtaUpdateManager::isReleaseApiConfigured() const
{
    const String endpoint = OTA_RELEASE_INFO_URL;
    if (endpoint.length() == 0)
    {
        return false;
    }

    if (endpoint.indexOf("OWNER") >= 0 || endpoint.indexOf("REPO") >= 0)
    {
        return false;
    }

    return true;
}

bool OtaUpdateManager::isConfigured() const
{
    return isReleaseApiConfigured();
}

bool OtaUpdateManager::fetchLatestReleaseJson(String &response, String &error) const
{
    if (WiFi.status() != WL_CONNECTED)
    {
        error = "Wi-Fi is not connected";
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, OTA_RELEASE_INFO_URL))
    {
        error = "Failed to initialize HTTPS client";
        return false;
    }

    http.setUserAgent("esp32-relay-ota-check");
    http.addHeader("Accept", "application/vnd.github+json");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    const int statusCode = http.GET();
    if (statusCode != HTTP_CODE_OK)
    {
        error = "Release API HTTP " + String(statusCode);
        http.end();
        return false;
    }

    response = http.getString();
    http.end();

    if (response.length() == 0)
    {
        error = "Release API returned empty body";
        return false;
    }

    return true;
}

bool OtaUpdateManager::parseLatestInfo(const String &json, String &version, String &assetUrl, String &error) const
{
    version = extractJsonString(json, "tag_name");
    if (version.length() == 0)
    {
        error = "Failed to parse tag_name from release metadata";
        return false;
    }

    String preferredAsset;
    const String desiredAssetName = OTA_FIRMWARE_ASSET_NAME;
    int searchIndex = 0;

    while (searchIndex >= 0)
    {
        const int keyIndex = json.indexOf("\"browser_download_url\"", searchIndex);
        if (keyIndex < 0)
        {
            break;
        }

        const String url = extractJsonString(json, "browser_download_url", keyIndex);
        if (url.length() > 0)
        {
            const bool isBinary = url.endsWith(".bin") || url.indexOf(".bin?") > 0;
            if (isBinary)
            {
                if (desiredAssetName.length() > 0 && url.indexOf(desiredAssetName) >= 0)
                {
                    preferredAsset = url;
                    break;
                }

                if (preferredAsset.length() == 0)
                {
                    preferredAsset = url;
                }
            }
        }

        searchIndex = keyIndex + 1;
    }

    if (preferredAsset.length() == 0)
    {
        error = "No firmware .bin asset found in latest release";
        return false;
    }

    assetUrl = preferredAsset;
    return true;
}

bool OtaUpdateManager::parseSemver(const String &value, int &major, int &minor, int &patch) const
{
    String normalized = value;
    normalized.trim();
    if (normalized.startsWith("v") || normalized.startsWith("V"))
    {
        normalized.remove(0, 1);
    }

    const int firstDot = normalized.indexOf('.');
    if (firstDot <= 0)
    {
        return false;
    }

    const int secondDot = normalized.indexOf('.', firstDot + 1);
    if (secondDot <= firstDot)
    {
        return false;
    }

    major = normalized.substring(0, firstDot).toInt();
    minor = normalized.substring(firstDot + 1, secondDot).toInt();

    String patchPart = normalized.substring(secondDot + 1);
    const int suffixSeparator = patchPart.indexOf('-');
    if (suffixSeparator > 0)
    {
        patchPart = patchPart.substring(0, suffixSeparator);
    }
    patch = patchPart.toInt();

    return true;
}

int OtaUpdateManager::compareSemver(const String &left, const String &right) const
{
    int leftMajor = 0;
    int leftMinor = 0;
    int leftPatch = 0;
    int rightMajor = 0;
    int rightMinor = 0;
    int rightPatch = 0;

    if (!parseSemver(left, leftMajor, leftMinor, leftPatch) ||
        !parseSemver(right, rightMajor, rightMinor, rightPatch))
    {
        return left.compareTo(right);
    }

    if (leftMajor != rightMajor)
    {
        return leftMajor < rightMajor ? -1 : 1;
    }

    if (leftMinor != rightMinor)
    {
        return leftMinor < rightMinor ? -1 : 1;
    }

    if (leftPatch != rightPatch)
    {
        return leftPatch < rightPatch ? -1 : 1;
    }

    return 0;
}

OtaCheckResult OtaUpdateManager::checkForUpdate()
{
    OtaCheckResult result;
    result.currentVersion = FIRMWARE_VERSION;
    result.configured = isReleaseApiConfigured();

    if (!result.configured)
    {
        result.message = "OTA release API is not configured in AppConfig.h";
        lastCheckOkFlag = false;
        updateAvailableFlag = false;
        latestVersionValue = "";
        selectedAssetUrlValue = "";
        lastMessageValue = result.message;
        return result;
    }

    String response;
    String error;
    if (!fetchLatestReleaseJson(response, error))
    {
        result.message = error;
        lastCheckOkFlag = false;
        updateAvailableFlag = false;
        latestVersionValue = "";
        selectedAssetUrlValue = "";
        lastMessageValue = result.message;
        return result;
    }

    String latestVersion;
    String assetUrl;
    if (!parseLatestInfo(response, latestVersion, assetUrl, error))
    {
        result.message = error;
        lastCheckOkFlag = false;
        updateAvailableFlag = false;
        latestVersionValue = "";
        selectedAssetUrlValue = "";
        lastMessageValue = result.message;
        return result;
    }

    result.ok = true;
    result.latestVersion = latestVersion;
    result.assetUrl = assetUrl;
    result.updateAvailable = compareSemver(result.currentVersion, latestVersion) < 0;
    result.message = result.updateAvailable ? "Update available" : "Device is already on latest version";

    lastCheckOkFlag = true;
    updateAvailableFlag = result.updateAvailable;
    latestVersionValue = latestVersion;
    selectedAssetUrlValue = assetUrl;
    lastMessageValue = result.message;

    return result;
}

bool OtaUpdateManager::performUpdate(String &message)
{
    const OtaCheckResult check = checkForUpdate();
    if (!check.ok)
    {
        message = check.message;
        return false;
    }

    if (!check.updateAvailable)
    {
        message = "No update required";
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        message = "Wi-Fi is not connected";
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    httpUpdate.rebootOnUpdate(false);

    String resolvedAssetUrl;
    String resolveError;
    if (!resolveFinalDownloadUrl(check.assetUrl, resolvedAssetUrl, resolveError))
    {
        message = "Update failed before download: " + resolveError;
        return false;
    }

    const t_httpUpdate_return updateResult = httpUpdate.update(client, resolvedAssetUrl);
    switch (updateResult)
    {
    case HTTP_UPDATE_OK:
        message = "Firmware updated successfully";
        return true;
    case HTTP_UPDATE_NO_UPDATES:
        message = "No update available from update endpoint";
        return false;
    case HTTP_UPDATE_FAILED:
    default:
        message = "Update failed: " + httpUpdate.getLastErrorString();
        return false;
    }
}

bool OtaUpdateManager::lastCheckOk() const
{
    return lastCheckOkFlag;
}

bool OtaUpdateManager::updateAvailable() const
{
    return updateAvailableFlag;
}

String OtaUpdateManager::currentVersion() const
{
    return FIRMWARE_VERSION;
}

String OtaUpdateManager::latestVersion() const
{
    return latestVersionValue;
}

String OtaUpdateManager::lastMessage() const
{
    return lastMessageValue;
}

String OtaUpdateManager::selectedAssetUrl() const
{
    return selectedAssetUrlValue;
}
