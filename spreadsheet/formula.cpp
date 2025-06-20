#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream &operator<<(std::ostream &output, FormulaError fe)
{
    return output << "#ARITHM!";
}

namespace
{
    class Formula : public FormulaInterface
    {
    public:
        explicit Formula(std::string expression)
        try : ast_(ParseFormulaAST(std::move(expression))) {}
        catch (const std::exception &e)
        {
            throw FormulaException(e.what());
        }

        Value Evaluate(const SheetInterface &sheet) const override
        {
            try
            {
                auto sheet_args = [&sheet](Position pos) -> double
                {
                    const auto *cell = sheet.GetCell(pos);
                    if (!cell)
                        return 0.0;

                    const auto &value = cell->GetValue();
                    if (std::holds_alternative<double>(value))
                    {
                        return std::get<double>(value);
                    }
                    if (std::holds_alternative<std::string>(value))
                    {
                        try
                        {
                            return std::stod(std::get<std::string>(value));
                        }
                        catch (...)
                        {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                    throw std::get<FormulaError>(value);
                };

                return ast_.Execute(sheet_args);
            }
            catch (const FormulaError &e)
            {
                return e;
            }
            catch (...)
            {
                return FormulaError(FormulaError::Category::Value);
            }
        }

        std::string GetExpression() const override
        {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const override
        {
            return ast_.GetReferencedCells();
        }

    private:
        FormulaAST ast_;
    };
} // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression)
{
    return std::make_unique<Formula>(std::move(expression));
}