#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>
#include <queue>

class Cell::Impl
{
public:
    virtual ~Impl() = default;
    virtual Value GetValue(const SheetInterface &sheet) const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const { return {}; }
};

class Cell::EmptyImpl : public Impl
{
public:
    Value GetValue(const SheetInterface &) const override { return ""; }
    std::string GetText() const override { return ""; }
};

class Cell::TextImpl : public Impl
{
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {}

    Value GetValue(const SheetInterface &) const override
    {
        return text_.front() == ESCAPE_SIGN ? text_.substr(1) : text_;
    }

    std::string GetText() const override
    {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl
{
public:
    explicit FormulaImpl(std::unique_ptr<FormulaInterface> formula)
        : formula_(std::move(formula)) {}

    Value GetValue(const SheetInterface &sheet) const override
    {
        auto result = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(result))
        {
            return std::get<double>(result);
        }
        return std::get<FormulaError>(result);
    }

    std::string GetText() const override
    {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override
    {
        return formula_->GetReferencedCells();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(SheetInterface &sheet, Position pos)
    : sheet_(sheet),
      pos_(pos),
      impl_(std::make_unique<EmptyImpl>()),
      dependent_cells_{},
      referenced_cells_{}
{
}

Cell::~Cell() = default;

void Cell::Set(std::string text)
{
    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN)
    {
        auto formula = ParseFormula(text.substr(1));
        CheckCircularDependency(formula->GetReferencedCells());
        impl_ = std::make_unique<FormulaImpl>(std::move(formula));
        UpdateDependencies(impl_->GetReferencedCells());
    }
    else
    {
        impl_ = std::make_unique<TextImpl>(std::move(text));
        UpdateDependencies({});
    }

    InvalidateCache();
}

void Cell::Clear()
{
    impl_ = std::make_unique<EmptyImpl>();
    UpdateDependencies({});
    InvalidateCache();
}

Cell::Value Cell::GetValue() const
{
    if (!cached_value_)
    {
        cached_value_ = impl_->GetValue(sheet_);
    }
    return *cached_value_;
}

std::string Cell::GetText() const
{
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

void Cell::InvalidateCache(bool invalidate_dependents)
{
    cached_value_.reset();

    if (invalidate_dependents)
    {
        std::unordered_set<Position> visited;
        std::queue<Position> to_invalidate;

        for (const auto &dep_pos : dependent_cells_)
        {
            to_invalidate.push(dep_pos);
        }

        while (!to_invalidate.empty())
        {
            Position current = to_invalidate.front();
            to_invalidate.pop();

            if (!visited.insert(current).second)
            {
                continue;
            }

            if (auto *cell = dynamic_cast<Cell *>(sheet_.GetCell(current)))
            {
                cell->cached_value_.reset();

                for (const auto &next_dep : cell->dependent_cells_)
                {
                    to_invalidate.push(next_dep);
                }
            }
        }
    }
}

void Cell::CheckCircularDependency(const std::vector<Position> &refs) const
{
    std::unordered_set<Position> visited;
    std::stack<Position> to_check;

    for (const auto &ref : refs)
    {
        to_check.push(ref);
    }

    while (!to_check.empty())
    {
        Position current = to_check.top();
        to_check.pop();

        if (current == pos_)
        {
            throw CircularDependencyException("Circular dependency detected");
        }

        if (!visited.insert(current).second)
        {
            continue;
        }

        if (const auto *cell = sheet_.GetCell(current))
        {
            for (const auto &ref : cell->GetReferencedCells())
            {
                to_check.push(ref);
            }
        }
    }
}

void Cell::UpdateDependencies(const std::vector<Position> &new_refs)
{
    for (const auto &old_ref : referenced_cells_)
    {
        if (auto *old_cell = dynamic_cast<Cell *>(sheet_.GetCell(old_ref)))
        {
            old_cell->dependent_cells_.erase(pos_);
        }
    }

    referenced_cells_.clear();

    for (const auto &new_ref : new_refs)
    {
        if (new_ref.IsValid())
        {
            referenced_cells_.insert(new_ref);
            if (auto *new_cell = dynamic_cast<Cell *>(sheet_.GetCell(new_ref)))
            {
                new_cell->dependent_cells_.insert(pos_);
            }
        }
    }
}