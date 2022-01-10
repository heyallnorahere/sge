using System;
using System.Collections.Generic;

namespace SGE.Setup
{
    internal static class Utilities
    {
        public static bool Extends(this Type derived, Type baseType)
        {
            Type? currentBaseType = derived.BaseType;
            if (currentBaseType != null)
            {
                if (currentBaseType == baseType)
                {
                    return true;
                }
                else
                {
                    return currentBaseType.Extends(baseType);
                }
            }

            return false;
        }

        public static string[] Tokenize(this string command)
        {
            string[] sections = command.Split(' ', StringSplitOptions.RemoveEmptyEntries);

            string? currentToken = null;
            var completedTokens = new List<string>();

            foreach (string section in sections)
            {
                bool containsQuote = false;
                bool escapeNextCharacter = false;

                string token = string.Empty;
                for (int i = 0; i < section.Length; i++)
                {
                    char character = section[i];

                    switch (character)
                    {
                        case '"':
                            if (escapeNextCharacter)
                            {
                                token += character;
                                escapeNextCharacter = false;
                            }
                            else
                            {
                                containsQuote = !containsQuote;
                            }
                            break;
                        case '\\':
                            if (i == section.Length - 1)
                            {
                                containsQuote = true;
                            }
                            else if (!escapeNextCharacter)
                            {
                                escapeNextCharacter = true;
                            }
                            else
                            {
                                escapeNextCharacter = false;
                                token += character;
                            }
                            break;
                        default:
                            if (escapeNextCharacter)
                            {
                                throw new ArgumentException($"cannot escape character: {character}");
                            }
                            else
                            {
                                token += character;
                            }
                            break;
                    }
                }

                bool beganToken = false;
                if (currentToken == null && containsQuote)
                {
                    currentToken = string.Empty;
                    beganToken = true;
                }

                if (currentToken != null)
                {
                    currentToken += " " + token;
                }
                else
                {
                    completedTokens.Add(token);
                }

                if (containsQuote && !beganToken && currentToken != null)
                {
                    completedTokens.Add(currentToken);
                    currentToken = null;
                }
            }

            if (currentToken != null)
            {
                completedTokens.Add(currentToken);
            }

            return completedTokens.ToArray();
        }
    }
}
