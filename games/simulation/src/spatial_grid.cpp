#include "spatial_grid.hpp"

namespace simulation {

SpatialGrid::SpatialGrid(float cell_size) : _cell_size(cell_size) {
    _entries.reserve(1024);
    _entry_indices.reserve(1024);
    _cells.reserve(256);
    _cell_indices.reserve(256);
    _active_cells.reserve(256);
}

void SpatialGrid::set_cell_size(float cell_size) {
    if (_cell_size == cell_size) {
        return;
    }

    _cell_size = cell_size;
    _entries.clear();
    _entry_indices.clear();
    _cells.clear();
    _cell_indices.clear();
    _active_cells.clear();
}

void SpatialGrid::clear() {
    for (const uint32_t index : _active_cells) {
        _cells[index].entry_indices.clear();
    }
    _active_cells.clear();
    _entries.clear();
    _entry_indices.clear();
}

void SpatialGrid::insert(
    ecs_entity_t entity,
    const Position3d &position,
    uint8_t team,
    float radius
) {
    const CellKey key = cell_for(position);
    const auto [cell, created] =
        _cell_indices.try_emplace(key, static_cast<uint32_t>(_cells.size()));
    if (created) {
        _cells.push_back({ key });
    }

    GridCell &grid_cell = _cells[cell->second];
    if (grid_cell.entry_indices.empty()) {
        _active_cells.push_back(cell->second);
    }

    const uint32_t index = static_cast<uint32_t>(_entries.size());
    _entries.push_back({ entity, position.x, position.y, position.z, radius, team });
    _entry_indices.emplace(entity, index);
    grid_cell.entry_indices.push_back(index);
}

const GridEntry *SpatialGrid::find(ecs_entity_t entity) const {
    const auto entry = _entry_indices.find(entity);
    return entry == _entry_indices.end() ? nullptr : &_entries[entry->second];
}

CellKey SpatialGrid::cell_for(const Position3d &position) const {
    return {
        static_cast<int32_t>(std::floor(position.x / _cell_size)),
        static_cast<int32_t>(std::floor(position.y / _cell_size)),
        static_cast<int32_t>(std::floor(position.z / _cell_size)),
    };
}

} // namespace simulation
