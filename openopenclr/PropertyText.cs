using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace openopenclr
{
    internal static class PropertyTextParser // Adapted from a different project, this format is an abomination
    {
        private struct ArrayStream
        {
            internal byte[] Array;
            internal int Position;

            public ArrayStream(byte[] array)
            {
                Array = array;
                Position = 0;
            }
        }

        static bool IsIdentifierEnd(char c) => c == '=' || c == ',' || c == 5 || c == 6 || c == 7 || c == 8;
        static char PeekStream(ArrayStream s)
        {
            if (s.Position >= s.Array.Length)
                return '\0';

            return (char)s.Array[s.Position];
        }

        static void SkipWhitespace(ArrayStream s)
        {
            while (Char.IsWhiteSpace(PeekStream(s)))
                s.Position++;
        }

        static void SkipCharacterAndWhitespace(ArrayStream s)
        {
            s.Position++;
            SkipWhitespace(s);
        }

        static string ReadIdentifier(ArrayStream s)
        {
            var sb = new StringBuilder();
            bool inString = false;

            while (true)
            {
                char c = PeekStream(s);
                if (c == '"')
                    inString = !inString;

                if ((!inString && (IsIdentifierEnd(c) || c == '\n')) || c == '\0')
                {
                    if (inString)
                        throw new InvalidDataException("Unclosed \" found");

                    SkipWhitespace(s);
                    return sb.ToString();
                }

                sb.Append(c);
                s.Position++;
            }
        }

        static void PreprocessPropertyData(byte[] data)
        {
            Stack<int> bracketStartIndices = new Stack<int>();
            Stack<byte> bracketEndChars = new Stack<byte>();

            bool inString = false;

            for (int i = 0; i < data.Length; ++i)
            {
                if (data[i] == '"')
                    inString = !inString;

                if (!inString)
                {
                    if (data[i] == '{')
                    {
                        data[i] = 5; // by default assume we're dealing with an array

                        bracketStartIndices.Push(i);
                        bracketEndChars.Push(6);
                    }

                    if (bracketStartIndices.Count > 0)
                    {
                        if (data[i] == ',')
                        {
                            int idx = bracketStartIndices.Peek();
                            if (data[idx] != 5)
                            {
                                data[idx] = 5; // '[' but replaced with 5 to avoid conflicts
                                bracketEndChars.Pop();
                                bracketEndChars.Push(6);
                            }
                        }
                        else if (data[i] == '=')
                        {
                            int idx = bracketStartIndices.Peek();
                            if (data[idx] != 7)
                            {
                                data[idx] = 7;
                                bracketEndChars.Pop();
                                bracketEndChars.Push(8);
                            }
                        }
                    }

                    if (data[i] == '}')
                    {
                        data[i] = bracketEndChars.Pop();
                        bracketStartIndices.Pop();
                    }
                }
            }

            if (inString)
                throw new InvalidDataException("Mismatched number of double-quote characters");

            if (bracketStartIndices.Count > 0)
                throw new InvalidDataException("Mismatched number of brackets");

            if (bracketEndChars.Count > 0)
                throw new InvalidDataException("Mismatched number of brackets");
        }

        static List<object> ReadArray(ArrayStream s)
        {
            var arr = new List<object>();

            while (true)
            {
                char nextChar = PeekStream(s);
                if (nextChar == 6)
                {
                    SkipCharacterAndWhitespace(s);
                    return arr;
                }
                else if (nextChar == ',')
                {
                    SkipCharacterAndWhitespace(s);
                }

                nextChar = PeekStream(s);
                if (nextChar == 7)
                {
                    SkipCharacterAndWhitespace(s);
                    arr.Add(ReadDictionary(s));
                }
                else if (nextChar == 5)
                {
                    SkipCharacterAndWhitespace(s);
                    arr.Add(ReadArray(s));
                }
                else
                {
                    arr.Add(ReadIdentifier(s));
                }
            }
        }

        static Dictionary<string, object> ReadDictionary(ArrayStream s)
        {
            var dict = new Dictionary<string, object>();

            while (true)
            {
                char nextChar = PeekStream(s);
                if (nextChar == 8 || nextChar == 0)
                {
                    SkipCharacterAndWhitespace(s);
                    return dict;
                }

                string key = ReadIdentifier(s);
                if (PeekStream(s) != '=')
                    throw new InvalidDataException($"Expected '=' after '{key}'");

                SkipCharacterAndWhitespace(s);
                nextChar = PeekStream(s);
                if (nextChar == 7)
                {
                    SkipCharacterAndWhitespace(s);
                    dict[key] = ReadDictionary(s);
                }
                else if (nextChar == 5)
                {
                    SkipCharacterAndWhitespace(s);
                    dict[key] = ReadArray(s);
                }
                else
                {
                    dict[key] = ReadIdentifier(s);
                }
            }
        }

        internal static Dictionary<string, object> ParseEntry(byte[] propertyData)
        {
            PreprocessPropertyData(propertyData);
            return ReadDictionary(new ArrayStream(propertyData));
        }
    }

    internal class PropertyText
    {
        internal Dictionary<string, object> PropertyData { get; }

        internal PropertyText(byte[] propertyText)
        {
            PropertyData = PropertyTextParser.ParseEntry(propertyText);
        }
    }
}
