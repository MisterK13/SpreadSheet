#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Cell : public CellInterface
{
public:
    Cell(SheetInterface &sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    SheetInterface &sheet_;
    Position pos_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cached_value_;

    std::unordered_set<Position, std::hash<Position>> dependent_cells_;
    std::unordered_set<Position, std::hash<Position>> referenced_cells_;

    void InvalidateCache(bool invalidate_cache = true);
    void UpdateDependencies(const std::vector<Position> &new_refs);
    void CheckCircularDependency(const std::vector<Position> &refs) const;
};