%option never-interactive yylineno reentrant stack noyywrap nounput noyy_top_state

%{

#include "intern.h"
#include "lex.yy.h"

void push_byte(char byte, int state, yyscan_t yyscanner);

%}

/* Scanner states */
%x statement
%x raw
%x comment
%x delimiter_start
%x delimiter_end

/* Definitions */
RAW_START                    "{"
RAW_END                      "}"
RAW_TOKEN                    "&"
SECTION_START                "#"
ISECTION_START               "^"
PARTIAL                      ">"
COMMENT                      "!"
SECTION_END                  "/"
WHITESPACE                   [ \t]
NEWLINE                      [\n\r]
ALPHA                        [a-zA-Z]
DIGIT                        [0-9]
IDENTIFIER                   ({ALPHA}|\.)({ALPHA}|{DIGIT}|[_./-])*
DELIMITER                    ({ALPHA}|{DIGIT}|[:punct:]|[<>\[\]|%@])+


/* Rules */
%%
<statement>=                         { yy_push_state(delimiter_start, yyscanner);             }
<delimiter_start>{WHITESPACE}*       {                                                        }
<delimiter_start>{DELIMITER}         { set_start_delimiter(yytext, (scanner_data_t*) yyget_extra(yyscanner));
                                       yy_pop_state(yyscanner);
                                       yy_push_state(delimiter_end, yyscanner);               }
<delimiter_end>{WHITESPACE}*         {                                                        }
<delimiter_end>{DELIMITER}           { set_next_end_delimiter(yytext, (scanner_data_t*) yyget_extra(yyscanner));             }
<delimiter_end>=                     { yy_pop_state(yyscanner);                               }
<statement>{RAW_TOKEN}               { push_token(TOKEN_RAW, yytext, yyscanner);              }
<statement>{RAW_START}               { yy_push_state(raw, yyscanner);
                                       push_token(TOKEN_RAW, yytext, yyscanner);              }
<raw>{RAW_END}                       { yy_pop_state(yyscanner);                               }
<statement,raw>{WHITESPACE}*         {                                                        }
<statement,raw>{SECTION_START}       { push_token(TOKEN_SECTION_START, yytext, yyscanner);    }
<statement,raw>{ISECTION_START}      { push_token(TOKEN_ISECTION_START, yytext, yyscanner);   }
<statement,raw>{SECTION_END}         { push_token(TOKEN_SECTION_END, yytext, yyscanner);      }
<statement,raw>{PARTIAL}             { push_token(TOKEN_PARTIAL, yytext, yyscanner);          }
<statement,raw>{IDENTIFIER}          { push_token(TOKEN_IDENTIFIER, yytext, yyscanner);       }
<statement>{COMMENT}                 { yy_pop_state(yyscanner);
                                       yy_push_state(comment, yyscanner);                     }
<comment>.|{WHITESPACE}|{NEWLINE}    { push_byte(yytext[0], COMMENT, yyscanner);              }
<statement>.                         { push_byte(yytext[0], STATEMENT, yyscanner);            }
{NEWLINE}                            { push_text(yyscanner);
                                       push_token(TOKEN_NEWLINE, yytext, yyscanner);          }
.|{WHITESPACE}                       { push_byte(yytext[0], TEXT, yyscanner);                 }
<<EOF>>                              { push_text(yyscanner);
                                       yyterminate();                                         }
%%

void push_byte(char byte, int state, yyscan_t yyscanner)
{
    scanner_data_t* data = (scanner_data_t*) yyget_extra(yyscanner);
    data->buffer[data->buffer_index] = byte;
    data->buffer[data->buffer_index + 1] = '\0';
    data->buffer_index++;

    // Check for starting or ending sections
    if (state == TEXT && is_string_in_buffer(data->statement_start, strlen(data->statement_start), data->buffer, data->buffer_index)) {
        // Section start
        data->buffer_index -= strlen(data->statement_start);
        data->buffer[data->buffer_index] = '\0';
        yy_push_state(statement, yyscanner);
        push_text(yyscanner);
    } else if (state != TEXT && is_string_in_buffer(data->statement_end, strlen(data->statement_end), data->buffer, data->buffer_index)) {
        // Section end
        data->buffer_index -= strlen(data->statement_end);
        data->buffer[data->buffer_index] = '\0';
        if (state == COMMENT) {
            push_comment(yyscanner);
        } else if (*data->next_statement_end) {
            use_next_end_delimiter(data);
        }

        yy_pop_state(yyscanner);
    }

    // Check for invalid tokens in the section
    if (state == STATEMENT && strncmp(data->statement_end, data->buffer, data->buffer_index) != 0) {
        push_error(yyscanner);
    }
}