#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <variant>

#include "asm_lexer.h"
#include "asm_parse.h"
#include "instruction_table_lexer.h"

int main() {
    auto table = BuildParserTable();

    AsmLexer lexer{std::cin};

    while (true) {
        printf("> ");
        auto line = GetLine(lexer);

        if (!line) {
            printf("Error during lex.\n\n");

            while (true) {
                auto token = lexer.NextToken();
                if (std::holds_alternative<AsmToken::EndOfFile>(token))
                    return 0;
                if (std::holds_alternative<AsmToken::EndOfLine>(token))
                    break;
            }

            continue;
        }

        if (line->empty() && std::holds_alternative<AsmToken::EndOfFile>(lexer.PeekToken())) {
            return 0;
        }

        bool parse_success = false;
        for (auto& parser : table) {
            if (auto result = parser.TryParse(*line)) {
                parse_success = true;

                printf("\nHex:\n");
                for (std::uint16_t v : *result) {
                    printf("%04x\n", v);
                }
                printf("\n");

                std::vector<std::uint16_t> message = *result;
                message.insert(message.begin(), 0xD590);

                break;
            }
        }
        if (!parse_success) {
            printf("Failed to parse previous input.\n\n");
        }
    }

    return 0;
}
