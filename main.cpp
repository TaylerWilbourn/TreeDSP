#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>

#include "asm_lexer.h"
#include "asm_parse.h"
#include "instruction_table_lexer.h"

const std::string instruction_table = 
#include "instruction_table.inc"
;

std::vector<InstructionParser> BuildParserTable() {
    std::vector<InstructionParser> table;

    std::istringstream stream{instruction_table};
    InstructionTableLexer lexer{stream};

    while (true) {
        while (lexer.PeekToken().type == InstructionTableToken::END_OF_LINE && lexer.PeekToken().type != InstructionTableToken::END_OF_FILE)
            lexer.NextToken();

        if (lexer.PeekToken().type == InstructionTableToken::END_OF_FILE)
            break;

        assert(lexer.PeekToken().type == InstructionTableToken::HEX);
        const std::uint16_t instruction_bits = std::strtol(lexer.NextToken().payload.c_str(), nullptr, 16);

        InstructionPartList part_list;

        while (lexer.PeekToken().type != InstructionTableToken::END_OF_LINE) {
            auto token = lexer.NextToken();
            assert(token.type == InstructionTableToken::IDENTIFIER);

            bool invert = false;

            const auto str_starts_with = [&](const std::string& haystack, const std::string& needle) -> bool {
                return haystack.compare(0, needle.length(), needle) == 0;
            };

            const auto parse_at_bit_pos = [&]() -> size_t {
                assert(lexer.NextToken().type == InstructionTableToken::AT);
                if (str_starts_with(lexer.PeekToken().payload, "not")) {
                    invert = true;
                    return std::strtol(lexer.NextToken().payload.c_str() + 3, nullptr, 10);
                }
                assert(lexer.PeekToken().type == InstructionTableToken::NUMBER);
                return std::strtol(lexer.NextToken().payload.c_str(), nullptr, 10);
            };

            const auto combine_with_previous = [&](std::shared_ptr<AsmInstructionPart> next) {
                assert(!invert); // invert doesn't make sense in this context
                part_list.back()->CombineWith(next);
            };

            const auto delete_comma_if_any = [&]() -> bool {
                if (lexer.PeekToken().payload == ",") {
                    lexer.NextToken();
                    return true;
                }
                if (dynamic_cast<TokenTypePart<AsmToken::Comma>*>(part_list.back().get()) != nullptr) {
                    part_list.pop_back();
                    return true;
                }
                return false;
            };

            if (token.payload == "Implied") {
                // Ignore
                continue;
            } else if (token.payload == "NoReverse") {
                assert(lexer.NextToken().payload == ",");
                continue;
            } else if (str_starts_with(token.payload, "Unused")) {
                parse_at_bit_pos(); // Ignore
                delete_comma_if_any();
                continue;
            } else if (token.payload == "Bogus") {
                while (lexer.PeekToken().payload != "||" && lexer.PeekToken().payload != "," && lexer.PeekToken().type != InstructionTableToken::END_OF_LINE) {
                    lexer.NextToken();
                }
                delete_comma_if_any();
                continue;
            } else if (token.payload == "||") {
                part_list.emplace_back(std::make_shared<TokenTypePart<AsmToken::DoublePipe>>());
            } else if (token.payload == "_") {
                part_list.emplace_back(std::make_shared<TokenTypePart<AsmToken::Colon>>());
            } else if (token.payload == ",") {
                part_list.emplace_back(std::make_shared<TokenTypePart<AsmToken::Comma>>());
            } else if (token.payload == "MemSp") {
                part_list.emplace_back(std::make_shared<MemSp>());
            } else if (token.payload == "MemR0") {
                part_list.emplace_back(std::make_shared<MemR0>());
            } else if (token.payload == "MemR01") {
                part_list.emplace_back(std::make_shared<MemR01>(parse_at_bit_pos()));
            } else if (token.payload == "MemR0123") {
                part_list.emplace_back(std::make_shared<MemR0123>(parse_at_bit_pos()));
            } else if (token.payload == "MemR04") {
                part_list.emplace_back(std::make_shared<MemR04>(parse_at_bit_pos()));
            } else if (token.payload == "MemR0425") {
                part_list.emplace_back(std::make_shared<MemR0425>(parse_at_bit_pos()));
            } else if (token.payload == "MemR45") {
                part_list.emplace_back(std::make_shared<MemR45>(parse_at_bit_pos()));
            } else if (token.payload == "MemR4567") {
                part_list.emplace_back(std::make_shared<MemR4567>(parse_at_bit_pos()));
            } else if (token.payload == "MemRn") {
                part_list.emplace_back(std::make_shared<MemRn>(parse_at_bit_pos()));
            } else if (token.payload == "ProgMemRn") {
                part_list.emplace_back(std::make_shared<ProgMemRn>(parse_at_bit_pos()));
            } else if (token.payload == "ProgMemR45") {
                part_list.emplace_back(std::make_shared<ProgMemR45>(parse_at_bit_pos()));
            } else if (token.payload == "ProgMemAxl") {
                part_list.emplace_back(std::make_shared<ProgMemAxl>(parse_at_bit_pos()));
            } else if (token.payload == "ProgMemAx") {
                part_list.emplace_back(std::make_shared<ProgMemAx>(parse_at_bit_pos()));
            } else if (token.payload == "MemImm8") {
                part_list.emplace_back(std::make_shared<MemImm8>(parse_at_bit_pos()));
            } else if (token.payload == "MemImm16") {
                part_list.emplace_back(std::make_shared<MemImm16>(parse_at_bit_pos()));
            } else if (token.payload == "MemR7Imm7s") {
                part_list.emplace_back(std::make_shared<MemR7Imm7s>(parse_at_bit_pos()));
            } else if (token.payload == "MemR7Imm16") {
                part_list.emplace_back(std::make_shared<MemR7Imm16>(parse_at_bit_pos()));
            } else if (token.payload == "BankFlags6") {
                part_list.emplace_back(std::make_shared<BankFlags6>(parse_at_bit_pos()));
            } else if (token.payload == "SwapTypes4") {
                part_list.emplace_back(std::make_shared<SwapTypes4>(parse_at_bit_pos()));
            } else if (token.payload == "Address16") {
                part_list.emplace_back(std::make_shared<Address16>(parse_at_bit_pos()));
            } else if (token.payload == "Address18") {
                assert(parse_at_bit_pos() == 16 && !invert);
                assert(str_starts_with(lexer.PeekToken().payload, "and"));
                const size_t i = std::strtol(lexer.NextToken().payload.c_str() + 3, nullptr, 10);
                part_list.emplace_back(std::make_shared<Address18>(i));
            } else if (token.payload == "RelAddr7") {
                part_list.emplace_back(std::make_shared<RelAddr7>(parse_at_bit_pos()));
            } else if (token.payload == "Imm2u") {
                part_list.emplace_back(std::make_shared<Imm2u>(parse_at_bit_pos()));
            } else if (token.payload == "Imm4") {
                part_list.emplace_back(std::make_shared<Imm4>(parse_at_bit_pos()));
            } else if (token.payload == "Imm4u") {
                part_list.emplace_back(std::make_shared<Imm4u>(parse_at_bit_pos()));
            } else if (token.payload == "Imm5s") {
                part_list.emplace_back(std::make_shared<Imm5s>(parse_at_bit_pos()));
            } else if (token.payload == "Imm5u") {
                part_list.emplace_back(std::make_shared<Imm5u>(parse_at_bit_pos()));
            } else if (token.payload == "Imm6s") {
                part_list.emplace_back(std::make_shared<Imm6s>(parse_at_bit_pos()));
            } else if (token.payload == "Imm7s") {
                part_list.emplace_back(std::make_shared<Imm7s>(parse_at_bit_pos()));
            } else if (token.payload == "Imm8") {
                part_list.emplace_back(std::make_shared<Imm8>(parse_at_bit_pos()));
            } else if (token.payload == "Imm8s") {
                part_list.emplace_back(std::make_shared<Imm8s>(parse_at_bit_pos()));
            } else if (token.payload == "Imm9u") {
                part_list.emplace_back(std::make_shared<Imm9u>(parse_at_bit_pos()));
            } else if (token.payload == "Imm8u") {
                part_list.emplace_back(std::make_shared<Imm8u>(parse_at_bit_pos()));
            } else if (token.payload == "Imm16") {
                part_list.emplace_back(std::make_shared<Imm16>(parse_at_bit_pos()));
            } else if (token.payload == "Imm4bitno") {
                part_list.emplace_back(std::make_shared<Imm4bitno>(parse_at_bit_pos()));
            } else if (token.payload == "stepZIDS") {
                part_list.emplace_back(std::make_shared<stepZIDS>(parse_at_bit_pos()));
            } else if (token.payload == "modrstepZIDS") {
                part_list.emplace_back(std::make_shared<modrstepZIDS>(parse_at_bit_pos()));
            } else if (token.payload == "stepII2D2S") {
                part_list.emplace_back(std::make_shared<stepII2D2S>(parse_at_bit_pos()));
            } else if (token.payload == "stepII2D2S0") {
                part_list.emplace_back(std::make_shared<stepII2D2S0>(parse_at_bit_pos()));
            } else if (token.payload == "modrstepII2D2S0") {
                part_list.emplace_back(std::make_shared<modrstepII2D2S0>(parse_at_bit_pos()));
            } else if (token.payload == "stepD2S") {
                part_list.emplace_back(std::make_shared<stepD2S>(parse_at_bit_pos()));
            } else if (token.payload == "stepII2") {
                part_list.emplace_back(std::make_shared<stepII2>(parse_at_bit_pos()));
            } else if (token.payload == "modrstepI2") {
                part_list.emplace_back(std::make_shared<modrstepI2>(parse_at_bit_pos()));
            } else if (token.payload == "modrstepD2") {
                part_list.emplace_back(std::make_shared<modrstepD2>(parse_at_bit_pos()));
            } else if (token.payload == "R0stepZIDS") {
                part_list.emplace_back(std::make_shared<SingleIdentifierPart>("r0"));
                part_list.emplace_back(std::make_shared<stepZIDS>(parse_at_bit_pos()));
            } else if (token.payload == "offsZI") {
                combine_with_previous(std::make_shared<offsZI>(parse_at_bit_pos()));
            } else if (token.payload == "offsI") {
                combine_with_previous(std::make_shared<offsI>());
            } else if (token.payload == "offsZIDZ") {
                combine_with_previous(std::make_shared<offsZIDZ>(parse_at_bit_pos()));
            } else if (token.payload == "Rn") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Rn, parse_at_bit_pos()));
            } else if (token.payload == "Ax") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Ax, parse_at_bit_pos()));
            } else if (token.payload == "Axl") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Axl, parse_at_bit_pos()));
            } else if (token.payload == "Axh") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Axh, parse_at_bit_pos()));
            } else if (token.payload == "Bx") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Bx, parse_at_bit_pos()));
            } else if (token.payload == "Bxl") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Bxl, parse_at_bit_pos()));
            } else if (token.payload == "Bxh") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Bxh, parse_at_bit_pos()));
            } else if (token.payload == "Ab") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Ab, parse_at_bit_pos()));
            } else if (token.payload == "Abl") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Abl, parse_at_bit_pos()));
            } else if (token.payload == "Abh") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Abh, parse_at_bit_pos()));
            } else if (token.payload == "Abe") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Abe, parse_at_bit_pos()));
            } else if (token.payload == "Px") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Px, parse_at_bit_pos()));
            } else if (token.payload == "Ablh") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Ablh, parse_at_bit_pos()));
            } else if (token.payload == "Cond") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Cond, parse_at_bit_pos()));
            } else if (token.payload == "Register") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Register, parse_at_bit_pos()));
            } else if (token.payload == "RegisterP0") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_RegisterP0, parse_at_bit_pos()));
            } else if (token.payload == "R0123457y0") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R0123457y0, parse_at_bit_pos()));
            } else if (token.payload == "R01") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R01, parse_at_bit_pos()));
            } else if (token.payload == "R04") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R04, parse_at_bit_pos()));
            } else if (token.payload == "R45") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R45, parse_at_bit_pos()));
            } else if (token.payload == "R0123") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R0123, parse_at_bit_pos()));
            } else if (token.payload == "R0425") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R0425, parse_at_bit_pos()));
            } else if (token.payload == "R4567") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_R4567, parse_at_bit_pos()));
            } else if (token.payload == "ArArpSttMod") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_ArArpSttMod, parse_at_bit_pos()));
            } else if (token.payload == "ArArp") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_ArArp, parse_at_bit_pos()));
            } else if (token.payload == "SttMod") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_SttMod, parse_at_bit_pos()));
            } else if (token.payload == "Ar") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Ar, parse_at_bit_pos()));
            } else if (token.payload == "Arp") {
                part_list.emplace_back(std::make_shared<SetOfIdentifierPart>(set_Arp, parse_at_bit_pos()));
            } else {
                part_list.emplace_back(std::make_shared<SingleIdentifierPart>(token.payload));
            }

            if (invert) {
                auto part = part_list.back();
                part_list.pop_back();
                part_list.emplace_back(std::make_shared<Not>(part));
            }
        }

        table.emplace_back(InstructionParser{instruction_bits, part_list});
    }

    return table;
}

void test_a() {
    std::istringstream stream{"mov [0x100], a1"};
    AsmLexer lexer{stream};

    auto line = GetLine(lexer);

    for (const auto& token : *line) {
        // printf("%s\n", AsmToken::ToString(token).c_str());
    }

    InstructionParser parser {
        0xD4FB,
        InstructionPartList {
            std::make_shared<SingleIdentifierPart>("mov"),
            std::make_shared<MemImm16>(16),
            std::make_shared<TokenTypePart<AsmToken::Comma>>(),
            std::make_shared<SetOfIdentifierPart>(set_Ax, 8),
        }
    };

    auto result = parser.TryParse(*line);

    assert(result);
    assert(result->size() == 2);
    assert(result->at(0) == 0xd5fb);
    assert(result->at(1) == 0x0100);
}

int main() {
    test_a();

    auto table = BuildParserTable();

    AsmLexer lexer{std::cin};

    auto line = GetLine(lexer);

    printf("\nTokens:\n");
    for (const auto& token : *line) {
        printf("%s\n", AsmToken::ToString(token).c_str());
    }

    printf("\nHex:\n");
    for (auto& parser : table) {
        if (auto result = parser.TryParse(*line)) {
            for (std::uint16_t v : *result) {
                printf("%04x\n", v);
            }
            break;
        }
    }

    return 0;
}
