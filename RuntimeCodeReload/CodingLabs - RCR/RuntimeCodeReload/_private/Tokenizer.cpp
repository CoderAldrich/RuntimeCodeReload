#include "Tokenizer.h"

using namespace RCR;

inline bool isLetter(char input)
{
    return (input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z');
}

inline bool isNumber(char input)
{
    return (input >= '0' && input <= '9');
}

inline bool isLiteral(char input)
{
    return isLetter(input) || isNumber(input) || input == '-' || input == '_' || input == '~';
}

Tokenizer::Tokenizer(const char* source, size_t sourceLength)
    : m_source(source)
    , m_sourceEnd(source + sourceLength)
    , m_input(source)
{}

void Tokenizer::reset()
{
    m_input = m_source;
}

Tokenizer::Token Tokenizer::nextToken()
{
    if(m_input)
    {
        // Check we are not at EOF
        if(m_input == m_sourceEnd)
            return Token(nullptr, 0, Token::eEnd);

        // Remove any leading white spaces
        if(*m_input == ' ' || *m_input == '\t')
        {
            do 
            {
                m_input++;
            } 
            while(m_input != m_sourceEnd && (*m_input == ' ' || *m_input == '\t'));
        }

        // Search next valid token
        size_t tokenSize = 0;
        const char* tokenStart = m_input;

        // Known tokens
        if(isLetter(*m_input))
        {
            bool hasScope = false;
            do 
            {
                hasScope |= *m_input == ':';
                m_input++; tokenSize++;
            } 
            while(m_input != m_sourceEnd && ( isLiteral(*m_input) || (*m_input == ':') ) );
            return Token(tokenStart, tokenSize, hasScope ? Token::eScopedType : Token::eString);
        }
        else if(*m_input == '[')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eOpenSquareBracket);
        }
        else if(*m_input == ']')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eClosedSquareBracket);
        }
        else if(*m_input == '(')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eOpenRoundBracket);
        }
        else if(*m_input == ')')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eClosedRoundBracket);
        }
        else if(*m_input == '<')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eOpenAngularBracket);
        }
        else if(*m_input == '>')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eClosedAngularBracket);
        }
        else if(*m_input == ',')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eComma);
        }
        else if(*m_input == '=')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eEqual);
        }
        else if(*m_input == '*')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eStar);
        }
        else if(*m_input == '&')
        {
            m_input++; tokenSize++;
            return Token(tokenStart, tokenSize, Token::eAmpersand);
        }

        // Unknown token
        m_input++; tokenSize++;
        return Token(tokenStart, tokenSize, Token::eUnknown);
    }

    // If no input, return an invalid token
    return Token();
}