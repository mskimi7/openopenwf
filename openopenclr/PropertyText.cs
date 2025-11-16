using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Newtonsoft.Json;

namespace openopenclr
{
    internal static class PropertyTextParser // Adapted from a different project, this format is an abomination
    {
        private class ArrayStream
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
            Stack<int> bracketStartIndexes = new Stack<int>();
            Stack<byte> bracketEndTypes = new Stack<byte>();

            bool isInString = false;

            for (int i = 0; i < data.Length; ++i)
            {
                if (data[i] == '"')
                    isInString = !isInString;

                if (!isInString)
                {
                    if (data[i] == '{')
                    {
                        data[i] = 5; // by default assume we're dealing with an array

                        bracketStartIndexes.Push(i);
                        bracketEndTypes.Push(6);
                    }

                    if (bracketStartIndexes.Count > 0)
                    {
                        if (data[i] == ',')
                        {
                            int idx = bracketStartIndexes.Peek();
                            if (data[idx] != 5)
                            {
                                data[idx] = 5; // '[' but replaced with 5 to avoid conflicts
                                bracketEndTypes.Pop();
                                bracketEndTypes.Push(6);
                            }
                        }
                        else if (data[i] == '=')
                        {
                            int idx = bracketStartIndexes.Peek();
                            if (data[idx] != 7)
                            {
                                data[idx] = 7;
                                bracketEndTypes.Pop();
                                bracketEndTypes.Push(8);
                            }
                        }
                    }

                    if (data[i] == '}')
                    {
                        data[i] = bracketEndTypes.Pop();
                        bracketStartIndexes.Pop();
                    }
                }
            }

            if (isInString)
                throw new InvalidDataException();

            if (bracketStartIndexes.Count > 0)
                throw new InvalidDataException();

            if (bracketEndTypes.Count > 0)
                throw new InvalidDataException();
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
                    throw new InvalidDataException($"Expected '=' after '{key}' but found '{PeekStream(s)}'");

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

        internal string TextData { get; }
        internal string JsonData { get; }

        internal PropertyText(byte[] propertyText)
        {
            TextData = Encoding.UTF8.GetString(propertyText); // technically, JSON is also text...
            TextData = TextData.Replace("\r\n", "\n").Replace("\n", "\r\n").Replace("\t", "  "); // make sure we're using the Windows line endings (for textbox to be happy)

            PropertyData = PropertyTextParser.ParseEntry(propertyText.ToArray()); // clone array
            JsonData = JsonConvert.SerializeObject(PropertyData, Formatting.Indented);
        }
    }
}
