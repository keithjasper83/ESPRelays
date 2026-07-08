#include "DeviceCommands.h"

#include "CommandRouter.h"
#include "RelayController.h"
#include "WiFiManager.h"

namespace
{
    CommandRouter *gRouter = nullptr;
    DeviceCommandContext *gContext = nullptr;

    const char *const ON_ALIASES[] = {"1"};
    const char *const OFF_ALIASES[] = {"0"};
    const char *const TOGGLE_ALIASES[] = {"t"};
    const char *const STATE_ALIASES[] = {"status", "?"};
    const char *const HELP_ALIASES[] = {"h", "commands"};

    void handleOn(const String &command)
    {
        (void)command;
        gContext->relay->set(true);
    }

    void handleOff(const String &command)
    {
        (void)command;
        gContext->relay->set(false);
    }

    void handleToggle(const String &command)
    {
        (void)command;
        gContext->relay->toggle();
    }

    void handleState(const String &command)
    {
        (void)command;
        if (gContext->printStatus != nullptr)
        {
            gContext->printStatus();
        }
    }

    void handleWifi(const String &command)
    {
        (void)command;
        gContext->wifi->printFullDetails();
    }

    void handleScan(const String &command)
    {
        (void)command;
        gContext->wifi->scanWifi();
    }

    void handleReconnect(const String &command)
    {
        (void)command;
        gContext->wifi->forceReconnect();
    }

    void handleHelp(const String &command)
    {
        (void)command;
        if (gRouter != nullptr)
        {
            gRouter->printHelp(Serial);
        }
    }
}

void DeviceCommands::begin(CommandRouter &router, DeviceCommandContext &context)
{
    gRouter = &router;
    gContext = &context;

    router.registerCommand({"on", ON_ALIASES, sizeof(ON_ALIASES) / sizeof(ON_ALIASES[0]), "turn the relay on", handleOn});
    router.registerCommand({"off", OFF_ALIASES, sizeof(OFF_ALIASES) / sizeof(OFF_ALIASES[0]), "turn the relay off", handleOff});
    router.registerCommand({"toggle", TOGGLE_ALIASES, sizeof(TOGGLE_ALIASES) / sizeof(TOGGLE_ALIASES[0]), "toggle the relay state", handleToggle});
    router.registerCommand({"state", STATE_ALIASES, sizeof(STATE_ALIASES) / sizeof(STATE_ALIASES[0]), "show relay, Wi-Fi, MQTT, and debug status", handleState});
    router.registerCommand({"wifi", nullptr, 0, "print full Wi-Fi details", handleWifi});
    router.registerCommand({"scan", nullptr, 0, "scan and print nearby Wi-Fi networks", handleScan});
    router.registerCommand({"reconnect", nullptr, 0, "force a Wi-Fi reconnect", handleReconnect});
    router.registerCommand({"help", HELP_ALIASES, sizeof(HELP_ALIASES) / sizeof(HELP_ALIASES[0]), "list available commands", handleHelp});
}