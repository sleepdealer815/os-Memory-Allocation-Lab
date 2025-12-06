#include "oslabs.h"

/* 工具函数：在内存映射数组中，在指定索引处插入一个新块。 */
void insert_at(struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int idx, struct MEMORY_BLOCK new_block) {
    // 从后向前移动，为新块腾出位置
    for (int i = *map_cnt; i > idx; i--) {
        memory_map[i] = memory_map[i - 1];
    }
    memory_map[idx] = new_block;
    (*map_cnt)++;
}

/* 工具函数：在内存映射数组中，移除指定索引处的块。 */
void remove_at_mem(struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int idx) {
    for (int i = idx; i < (*map_cnt) - 1; i++) {
        memory_map[i] = memory_map[i + 1];
    }
    (*map_cnt)--;
}

/* =====================================================================
   =======================   BEST FIT ALLOCATE   =======================
   ===================================================================== */
struct MEMORY_BLOCK best_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id) {
    struct MEMORY_BLOCK NULLBLOCK = {0, 0, 0, 0};
    int best_idx = -1;
    int best_size_diff = 0; // 记录最佳匹配块与请求大小的差值（正值）

    // 1. 遍历，寻找满足条件且大小最接近请求的空闲块
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            int diff = memory_map[i].segment_size - request_size;
            if (best_idx == -1 || diff < best_size_diff) {
                best_idx = i;
                best_size_diff = diff;
            }
        }
    }

    // 没有找到合适的空闲块
    if (best_idx == -1) {
        return NULLBLOCK;
    }

    struct MEMORY_BLOCK *target = &memory_map[best_idx];

    // 2. 情况A：大小完全相等 -> 直接分配
    if (target->segment_size == request_size) {
        target->process_id = process_id;
        return *target;
    }

    // 3. 情况B：空闲块更大 -> 需要拆分
    // 3.1 创建并插入新的空闲块（后半部分）
    struct MEMORY_BLOCK new_free_block;
    new_free_block.start_address = target->start_address + request_size;
    new_free_block.end_address = target->end_address;
    new_free_block.segment_size = target->segment_size - request_size;
    new_free_block.process_id = 0; // 空闲

    insert_at(memory_map, map_cnt, best_idx + 1, new_free_block);

    // 3.2 修改原块为已分配块（前半部分）
    target->end_address = target->start_address + request_size - 1;
    target->segment_size = request_size;
    target->process_id = process_id;

    return *target;
}

/* =====================================================================
   =======================  FIRST FIT ALLOCATE   =======================
   ===================================================================== */
struct MEMORY_BLOCK first_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id) {
    struct MEMORY_BLOCK NULLBLOCK = {0, 0, 0, 0};

    // 1. 找到第一个满足条件（空闲且足够大）的块
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            struct MEMORY_BLOCK *target = &memory_map[i];

            // 2. 情况A：大小完全相等 -> 直接分配
            if (target->segment_size == request_size) {
                target->process_id = process_id;
                return *target;
            }

            // 3. 情况B：需要拆分
            // 3.1 创建并插入新的空闲块
            struct MEMORY_BLOCK new_free_block;
            new_free_block.start_address = target->start_address + request_size;
            new_free_block.end_address = target->end_address;
            new_free_block.segment_size = target->segment_size - request_size;
            new_free_block.process_id = 0;

            insert_at(memory_map, map_cnt, i + 1, new_free_block);

            // 3.2 修改原块为已分配块
            target->end_address = target->start_address + request_size - 1;
            target->segment_size = request_size;
            target->process_id = process_id;

            return *target;
        }
    }
    // 没有找到合适的块
    return NULLBLOCK;
}

/* =====================================================================
   =======================  WORST FIT ALLOCATE   =======================
   ===================================================================== */
struct MEMORY_BLOCK worst_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id) {
    struct MEMORY_BLOCK NULLBLOCK = {0, 0, 0, 0};
    int worst_idx = -1;
    int worst_size = 0; // 记录最大的空闲块大小

    // 1. 遍历，寻找最大的空闲块
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            if (worst_idx == -1 || memory_map[i].segment_size > worst_size) {
                worst_idx = i;
                worst_size = memory_map[i].segment_size;
            }
        }
    }

    if (worst_idx == -1) {
        return NULLBLOCK;
    }

    struct MEMORY_BLOCK *target = &memory_map[worst_idx];

    // 2. 情况A：大小完全相等 -> 直接分配
    if (target->segment_size == request_size) {
        target->process_id = process_id;
        return *target;
    }

    // 3. 情况B：需要拆分
    struct MEMORY_BLOCK new_free_block;
    new_free_block.start_address = target->start_address + request_size;
    new_free_block.end_address = target->end_address;
    new_free_block.segment_size = target->segment_size - request_size;
    new_free_block.process_id = 0;

    insert_at(memory_map, map_cnt, worst_idx + 1, new_free_block);

    target->end_address = target->start_address + request_size - 1;
    target->segment_size = request_size;
    target->process_id = process_id;

    return *target;
}

/* =====================================================================
   =======================  NEXT FIT ALLOCATE   ========================
   ===================================================================== */
struct MEMORY_BLOCK next_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id, int last_address) {
    struct MEMORY_BLOCK NULLBLOCK = {0, 0, 0, 0};
    int start_idx = -1;

    // 1. 找到起始搜索索引（起始地址 >= last_address 的第一个块）
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].start_address >= last_address) {
            start_idx = i;
            break;
        }
    }
    // 如果没找到（last_address 超过最大地址），则从头开始
    if (start_idx == -1) {
        start_idx = 0;
    }

    // 2. 执行“环形”搜索：从 start_idx 搜到末尾，再从头搜到 start_idx-1
    for (int offset = 0; offset < *map_cnt; offset++) {
        int i = (start_idx + offset) % (*map_cnt);
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            struct MEMORY_BLOCK *target = &memory_map[i];

            if (target->segment_size == request_size) {
                target->process_id = process_id;
                return *target;
            }

            // 需要拆分
            struct MEMORY_BLOCK new_free_block;
            new_free_block.start_address = target->start_address + request_size;
            new_free_block.end_address = target->end_address;
            new_free_block.segment_size = target->segment_size - request_size;
            new_free_block.process_id = 0;

            insert_at(memory_map, map_cnt, i + 1, new_free_block);

            target->end_address = target->start_address + request_size - 1;
            target->segment_size = request_size;
            target->process_id = process_id;

            return *target;
        }
    }

    return NULLBLOCK;
}

/* =====================================================================
   =======================    RELEASE MEMORY    ========================
   ===================================================================== */
void release_memory(struct MEMORY_BLOCK freed_block, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt) {
    // 1. 在内存映射中找到要释放的块（根据起始地址匹配）
    int idx = -1;
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].start_address == freed_block.start_address) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        return; // 未找到，直接返回
    }

    // 2. 标记该块为空闲
    memory_map[idx].process_id = 0;

    // 3. 向前合并：如果前一个块存在且为空闲
    if (idx > 0 && memory_map[idx - 1].process_id == 0) {
        // 扩展前一个块到当前块的末尾
        memory_map[idx - 1].end_address = memory_map[idx].end_address;
        memory_map[idx - 1].segment_size = memory_map[idx - 1].end_address - memory_map[idx - 1].start_address + 1;
        // 移除当前块
        remove_at_mem(memory_map, map_cnt, idx);
        idx--; // 更新索引，指向合并后的块
    }

    // 4. 向后合并：如果后一个块存在且为空闲
    if (idx < (*map_cnt) - 1 && memory_map[idx + 1].process_id == 0) {
        // 扩展当前块到后一个块的末尾
        memory_map[idx].end_address = memory_map[idx + 1].end_address;
        memory_map[idx].segment_size = memory_map[idx].end_address - memory_map[idx].start_address + 1;
        // 移除后一个块
        remove_at_mem(memory_map, map_cnt, idx + 1);
    }
}
