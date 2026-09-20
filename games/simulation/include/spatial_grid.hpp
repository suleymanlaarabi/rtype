#pragma once

#include <siecs.h>
#include <siecs_spatial.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace simulation {

struct GridEntry {
    ecs_entity_t entity;
    float x;
    float y;
    float z;
    float radius;
    uint8_t team;
};

struct CellKey {
    int32_t x;
    int32_t y;
    int32_t z;

    bool operator==(const CellKey &) const = default;
};

struct CellKeyHash {
    std::size_t operator()(const CellKey &key) const noexcept {
        const uint64_t x = static_cast<uint32_t>(key.x) * 73856093U;
        const uint64_t y = static_cast<uint32_t>(key.y) * 19349663U;
        const uint64_t z = static_cast<uint32_t>(key.z) * 83492791U;
        return static_cast<std::size_t>(x ^ y ^ z);
    }
};

struct GridCell {
    CellKey key;
    std::vector<uint32_t> entry_indices;
};

class SpatialGrid {
  public:
    explicit SpatialGrid(float cell_size = 12.0f);

    void set_cell_size(float cell_size);
    void clear();
    void insert(ecs_entity_t entity, const Position3d &position, uint8_t team, float radius);
    const GridEntry *find(ecs_entity_t entity) const;

    template <typename F>
    void for_nearby(const Position3d &position, float radius, F &&callback) const {
        const CellKey center = cell_for(position);
        const int32_t extent = static_cast<int32_t>(std::ceil(radius / _cell_size));

        for (int32_t z = center.z - extent; z <= center.z + extent; ++z) {
            for (int32_t y = center.y - extent; y <= center.y + extent; ++y) {
                for (int32_t x = center.x - extent; x <= center.x + extent; ++x) {
                    const auto cell = _cell_indices.find({ x, y, z });
                    if (cell == _cell_indices.end()) {
                        continue;
                    }

                    for (const uint32_t index : _cells[cell->second].entry_indices) {
                        callback(_entries[index]);
                    }
                }
            }
        }
    }

    template <typename F>
    void for_populated_nearby(const Position3d &position, float radius, F &&callback) const {
        const float radius_squared = radius * radius;

        for (const uint32_t index : _active_cells) {
            const GridCell &cell = _cells[index];
            const float minimum_x = static_cast<float>(cell.key.x) * _cell_size;
            const float minimum_y = static_cast<float>(cell.key.y) * _cell_size;
            const float minimum_z = static_cast<float>(cell.key.z) * _cell_size;
            const float maximum_x = minimum_x + _cell_size;
            const float maximum_y = minimum_y + _cell_size;
            const float maximum_z = minimum_z + _cell_size;
            const float dx = std::clamp(position.x, minimum_x, maximum_x) - position.x;
            const float dy = std::clamp(position.y, minimum_y, maximum_y) - position.y;
            const float dz = std::clamp(position.z, minimum_z, maximum_z) - position.z;

            if (dx * dx + dy * dy + dz * dz > radius_squared) {
                continue;
            }

            for (const uint32_t entry_index : cell.entry_indices) {
                callback(_entries[entry_index]);
            }
        }
    }

    [[nodiscard]] uint32_t active_cell_count() const {
        return static_cast<uint32_t>(_active_cells.size());
    }

  private:
    CellKey cell_for(const Position3d &position) const;

    float _cell_size;
    std::vector<GridEntry> _entries;
    std::unordered_map<ecs_entity_t, uint32_t> _entry_indices;
    std::vector<GridCell> _cells;
    std::unordered_map<CellKey, uint32_t, CellKeyHash> _cell_indices;
    std::vector<uint32_t> _active_cells;
};

} // namespace simulation
