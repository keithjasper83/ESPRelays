/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include <unity.h>

#include "Arduino.h"
#include "CommandRouter.h"

#include "../../src/CommandRouter.cpp"

namespace
{
    int gOnCallCount = 0;
    int gHelpCallCount = 0;

    const char *const ON_ALIASES[] = {"1", "enable"};

    void onHandler(const String &)
    {
        gOnCallCount++;
    }

    void helpHandler(const String &)
    {
        gHelpCallCount++;
    }
}

void setUp()
{
    gOnCallCount = 0;
    gHelpCallCount = 0;
}

void tearDown()
{
}

void test_dispatch_matches_name_case_insensitive()
{
    CommandRouter router;
    TEST_ASSERT_TRUE(router.registerCommand({"on", ON_ALIASES, 2, "turn on", onHandler}));

    TEST_ASSERT_TRUE(router.dispatch("ON"));
    TEST_ASSERT_EQUAL(1, gOnCallCount);
}

void test_dispatch_matches_alias_case_insensitive()
{
    CommandRouter router;
    TEST_ASSERT_TRUE(router.registerCommand({"on", ON_ALIASES, 2, "turn on", onHandler}));

    TEST_ASSERT_TRUE(router.dispatch("EnAbLe"));
    TEST_ASSERT_EQUAL(1, gOnCallCount);
}

void test_dispatch_unknown_returns_false()
{
    CommandRouter router;
    TEST_ASSERT_TRUE(router.registerCommand({"on", ON_ALIASES, 2, "turn on", onHandler}));

    TEST_ASSERT_FALSE(router.dispatch("missing-command"));
    TEST_ASSERT_EQUAL(0, gOnCallCount);
}

void test_register_rejects_null_name_or_handler()
{
    CommandRouter router;
    TEST_ASSERT_FALSE(router.registerCommand({nullptr, ON_ALIASES, 2, "invalid", onHandler}));
    TEST_ASSERT_FALSE(router.registerCommand({"bad", ON_ALIASES, 2, "invalid", nullptr}));
    TEST_ASSERT_EQUAL(0u, router.count());
}

void test_register_max_commands_limit()
{
    CommandRouter router;

    for (int i = 0; i < 16; i++)
    {
        TEST_ASSERT_TRUE(router.registerCommand({"x", nullptr, 0, "ok", onHandler}));
    }

    TEST_ASSERT_FALSE(router.registerCommand({"overflow", nullptr, 0, "no", onHandler}));
    TEST_ASSERT_EQUAL(16u, router.count());
}

void test_print_help_includes_command_and_aliases()
{
    CommandRouter router;
    TEST_ASSERT_TRUE(router.registerCommand({"on", ON_ALIASES, 2, "turn on", onHandler}));
    TEST_ASSERT_TRUE(router.registerCommand({"help", nullptr, 0, "show help", helpHandler}));

    Stream output;
    router.printHelp(output);

    const std::string text = output.buffer();
    TEST_ASSERT_NOT_EQUAL(std::string::npos, text.find("Available commands:"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, text.find("on"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, text.find("enable"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, text.find("turn on"));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_dispatch_matches_name_case_insensitive);
    RUN_TEST(test_dispatch_matches_alias_case_insensitive);
    RUN_TEST(test_dispatch_unknown_returns_false);
    RUN_TEST(test_register_rejects_null_name_or_handler);
    RUN_TEST(test_register_max_commands_limit);
    RUN_TEST(test_print_help_includes_command_and_aliases);
    return UNITY_END();
}
