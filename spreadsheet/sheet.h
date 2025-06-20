#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <optional>

class Sheet : public SheetInterface
{
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface *GetCell(Position pos) const override;
    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;
    void PrintTexts(std::ostream &output) const override;

private:
    std::unordered_map<Position, std::unique_ptr<Cell>> cells_;
    mutable std::optional<Size> printable_size_cache_;

    void ValidatePosition(Position pos) const;
    void InvalidateCache() const;
    Size CalculatePrintableSize() const;
};