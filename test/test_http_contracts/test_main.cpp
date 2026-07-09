#include <fstream>
#include <string>
#include <unity.h>

namespace
{
    bool readFileText(const char *path, std::string &out)
    {
        std::ifstream input(path, std::ios::in | std::ios::binary);
        if (!input.is_open())
        {
            return false;
        }

        out.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        return true;
    }

    std::string readWorkspaceFile(const char *relativePath)
    {
        static const char *const prefixes[] = {"", "../", "../../", "../../../"};
        std::string text;
        for (const char *prefix : prefixes)
        {
            const std::string candidate = std::string(prefix) + relativePath;
            if (readFileText(candidate.c_str(), text))
            {
                return text;
            }
        }

        return std::string();
    }
}

void test_required_post_routes_exist()
{
    const std::string web = readWorkspaceFile("src/WebControlServer.cpp");
    TEST_ASSERT_FALSE(web.empty());

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/config\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/time/config\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/time/sync\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/schedule/add\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/schedule/update\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/schedule/delete\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/ota/check\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/ota/update\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/hostname\", HTTP_POST"));
}

void test_config_handler_reads_form_fields()
{
    const std::string web = readWorkspaceFile("src/WebControlServer.cpp");
    TEST_ASSERT_FALSE(web.empty());

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"wifi_ssid\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"wifi_pass\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"mqtt_host\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"mqtt_port\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"mqtt_enabled\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"hostname\")"));
}

void test_time_and_schedule_handlers_read_form_fields_and_validate()
{
    const std::string web = readWorkspaceFile("src/WebControlServer.cpp");
    TEST_ASSERT_FALSE(web.empty());

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"time_enabled\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"time_server\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"time_timezone\")"));

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"enabled\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"recurring\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"command\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"time_hm\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"dow_mask\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.arg(\"date_ymd\")"));

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("Invalid dow_mask. Use 0-127 with Monday as bit 0"));
}

void test_json_status_endpoints_exist()
{
    const std::string web = readWorkspaceFile("src/WebControlServer.cpp");
    TEST_ASSERT_FALSE(web.empty());

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/status\", HTTP_GET"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/config\", HTTP_GET"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/time\", HTTP_GET"));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_required_post_routes_exist);
    RUN_TEST(test_config_handler_reads_form_fields);
    RUN_TEST(test_time_and_schedule_handlers_read_form_fields_and_validate);
    RUN_TEST(test_json_status_endpoints_exist);
    return UNITY_END();
}
