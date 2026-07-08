#pragma once

#include <Arduino.h>

class CommandRouter
{
public:
    using CommandHandler = void (*)(const String &command);

    struct CommandDefinition
    {
        const char *name;
        const char *const *aliases;
        size_t aliasCount;
        const char *description;
        CommandHandler handler;
    };

    bool registerCommand(const CommandDefinition &definition);
    bool dispatch(const String &input) const;
    void printHelp(Stream &output) const;
    size_t count() const;

private:
    static constexpr size_t MAX_COMMANDS = 16;

    const CommandDefinition *findCommand(const String &input) const;

    CommandDefinition commands[MAX_COMMANDS] = {};
    size_t commandCount = 0;
};