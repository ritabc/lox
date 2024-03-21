//
// Created by Rita Bennett-Chew on 2/7/24.
//

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(VM* vm, Table* table) {
    FREE_ARRAY(vm, Entry, table->entries, table->capacity);
    initTable(table);
}

// Given a key and an array of buckets, which bucket does the entry belong to?
// Use modulo capacity to divide entries into buckets (index)
// Also use open addressing - find an entry for lookup (or empty slot for inserting) that either corresponds to:
// - the key's hash
// - or, if that's full, then the next slot where we don't already have an entry
// Return an Entry (comprised of key & value)
// Returns entry with key = NULL if no entry found with given key. This'll either be a totally empty entry or the first tombstone (key=NULL, value=TrueValue) encountered
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash & (capacity - 1);
    Entry* tombstone = NULL;
    for (;;) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // empty entry (key and value are empty)
                // if we've encountered any tombstones, return the first one (for reuse)
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone (key is NULL but value exists)
                // save the first tombstone encountered
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

static void adjustCapacity(VM* vm, Table* table, int capacity) {
    Entry* entries = ALLOCATE(vm, Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // we're resizing to a new capacity
    // we'll soon re-insert all entries to a new array
    // don't copy over tombstones
    table->count = 0;

    // To choose which bucket for each entry, we take its hash key % array size
    // So when size changes, we'll have to re-insert every entry into the new empty array
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    // Release memory for old array
    FREE_ARRAY(vm, Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

// Sets a value to a key in the table
// Before tableSet is called, we ensure the ObjString* key we pass is interned,
// meaning it'll be either a textually new string,
// or a pointer to a textually equivalent string.
bool tableSet(VM* vm, Table* table, ObjString* key, Value value) {
    // allocate a big-enough array
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(vm, table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;

    // increment only if it's a totally new entry - no existing key, not a tombstone
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

// Replace the entry with key 'key', if found, with tombstone (key=NULL, value=TrueValue)
// return true if an entry was found and replaced with a tombstone.
bool tableDelete(Table* table, ObjString* key) {
    if (table->count == 0) return false;

    // Find the entry
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    // Place a tombstone in the entry
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

// Used with method inheritance.
// Walks the bucket array of source table
// When encounters a non-empty bucket,
// adds the entry to the dest hash table using tableSet()
void tableAddAll(VM* vm, Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(vm, to, entry->key, entry->value);
        }
    }
}

// similar to findEntry, but we can't use that because tableFindString() was created to find interned strings in vm.strings, which is our solution to findEntry()'s problem of comparing key strings by total equality. (Before interning, 2 strings could be textually equal but wouldn't be found to be equal b/c they'd have different locations)
// Compare strings char-by-char here, and only here, and the rest of the VM can take advantage of that - taking for granted that any 2 strings at different addresses in memory must have different contents
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash & (table->capacity - 1);
    for (;;) {
        Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length
                    && entry->key->hash == hash
                    && memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }

        index = (index+1) & (table->capacity - 1);
    }
}

void tableRemoveWhite(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(VM* vm, Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        markObject(vm,(Obj*)entry->key);
        markValue(vm, entry->value);
    }
}