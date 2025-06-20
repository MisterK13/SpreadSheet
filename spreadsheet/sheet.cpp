#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::ValidatePosition(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }
}

void Sheet::InvalidateCache() const
{
    printable_size_cache_.reset();
}

Size Sheet::CalculatePrintableSize() const
{
    if (cells_.empty())
    {
        return {0, 0};
    }

    int max_row = 0;
    int max_col = 0;

    for (const auto &[pos, ptr] : cells_)
    {
        max_row = std::max(max_row, pos.row);
        max_col = std::max(max_col, pos.col);
    }

    return {max_row + 1, max_col + 1};
}

void Sheet::SetCell(Position pos, std::string text)
{
    ValidatePosition(pos);

    auto &cell_ptr = cells_[pos];
    if (!cell_ptr)
    {
        cell_ptr = std::make_unique<Cell>(*this, pos);
    }

    try
    {
        cell_ptr->Set(std::move(text));
        InvalidateCache();
    }
    catch (const FormulaException &)
    {
        cells_.erase(pos);
        InvalidateCache();
        throw;
    }
}

const CellInterface *Sheet::GetCell(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    auto it = cells_.find(pos);
    return it != cells_.end() ? it->second.get() : nullptr;
}

CellInterface *Sheet::GetCell(Position pos)
{
    return const_cast<CellInterface *>(
        static_cast<const Sheet &>(*this).GetCell(pos));
}

void Sheet::ClearCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    if (cells_.erase(pos))
    {
        InvalidateCache();
    }
}

Size Sheet::GetPrintableSize() const
{
    if (!printable_size_cache_)
    {
        printable_size_cache_ = CalculatePrintableSize();
    }
    return *printable_size_cache_;
}

void Sheet::PrintValues(std::ostream &output) const
{
    auto size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row)
    {
        for (int col = 0; col < size.cols; ++col)
        {
            if (col > 0)
            {
                output << '\t';
            }

            auto it = cells_.find({row, col});
            if (it != cells_.end())
            {
                const auto &value = it->second->GetValue();
                if (std::holds_alternative<double>(value))
                {
                    output << std::get<double>(value);
                }
                else if (std::holds_alternative<std::string>(value))
                {
                    output << std::get<std::string>(value);
                }
                else
                {
                    output << std::get<FormulaError>(value);
                }
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream &output) const
{
    auto size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row)
    {
        for (int col = 0; col < size.cols; ++col)
        {
            if (col > 0)
            {
                output << '\t';
            }

            auto it = cells_.find({row, col});
            if (it != cells_.end())
            {
                output << it->second->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet()
{
    return std::make_unique<Sheet>();
}