#include "CommandRouter.h"

namespace
{
    bool matches(const String &input, const char *candidate)
    {
        if (candidate == nullptr)
        {
            return false;
        }

        return input.equalsIgnoreCase(candidate);
    }
}

bool CommandRouter::registerCommand(const CommandDefinition &definition)
{
    if (commandCount >= MAX_COMMANDS || definition.name == nullptr || definition.handler == nullptr)
    {
        return false;
    }

    commands[commandCount++] = definition;
    return true;
}

const CommandRouter::CommandDefinition *CommandRouter::findCommand(const String &input) const
{
    for (size_t index = 0; index < commandCount; index++)
    {
        const CommandDefinition &definition = commands[index];

        if (matches(input, definition.name))
        {
            return &definition;
        }

        for (size_t aliasIndex = 0; aliasIndex < definition.aliasCount; aliasIndex++)
        {
            if (matches(input, definition.aliases[aliasIndex]))
            {
                return &definition;
            }
        }
    }

    return nullptr;
}

bool CommandRouter::dispatch(const String &input) const
{
    const CommandDefinition *definition = findCommand(input);
    if (definition == nullptr)
    {
        return false;
    }

    definition->handler(input);
    return true;
}

void CommandRouter::printHelp(Stream &output) const
{
    output.println("Available commands:");

    for (size_t index = 0; index < commandCount; index++)
    {
        const CommandDefinition &definition = commands[index];

        output.print("- ");
        output.print(definition.name);

        if (definition.aliasCount > 0)
        {
            output.print(" (");
            for (size_t aliasIndex = 0; aliasIndex < definition.aliasCount; aliasIndex++)
            {
                if (aliasIndex > 0)
                {
                    output.print(", ");
                }
                output.print(definition.aliases[aliasIndex]);
            }
            output.print(")");
        }

        output.print(" - ");
        output.println(definition.description != nullptr ? definition.description : "");
    }
}

size_t CommandRouter::count() const
{
    return commandCount;
}