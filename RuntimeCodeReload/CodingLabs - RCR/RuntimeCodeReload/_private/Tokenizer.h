#pragma once
#include <string>

namespace RCR
{
    class Tokenizer
    {
    public:
        // Structure
        //------------------------------------------------------------
        class Token
        {
        public:
            enum Type
            {
                eUnknown,
                eOpenRoundBracket,
                eClosedRoundBracket,
                eOpenSquareBracket,
                eClosedSquareBracket,
                eOpenAngularBracket,
                eClosedAngularBracket,
                eString,
                eScopedType,
                eComma,
                eStar,
                eAmpersand,
                eEqual,
                eEnd,
            };

            Token(const char* token, size_t length, Type type) : m_token(token), m_type(type), m_length(length) {};
            Token() : m_token(nullptr), m_type(eUnknown) {};
            bool operator == (const char* c){ return strncmp(c,m_token,m_length) == 0; }
            bool operator != (const char* c){ return strncmp(c,m_token,m_length) != 0; }
            bool operator == (Type c){ return c == m_type; }
            bool operator != (Type c){ return c != m_type; }

            operator std::string () const { return std::string(m_token, m_token + m_length); }
            
            size_t stringLength() const { return m_length; }

        private:
            friend class Tokenizer;
            const char*     m_token;
            Type            m_type;
            size_t          m_length;
        };

        // Ctors/Dtors
        //------------------------------------------------------------
        Tokenizer(const char* source, size_t sourceLength);

        // Methods
        //------------------------------------------------------------
        void    reset();
        Token   nextToken();

    private:
        // Attributes
        //------------------------------------------------------------
        const char*     m_source;
        const char*     m_sourceEnd;
        const char*     m_input;
    };
}