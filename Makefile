# ══════════════════════════════════════════════════════════
#  MockAI – Makefile
#  Ghali (Backend C Lead) + Ahmed (Database & Cloud)
# ══════════════════════════════════════════════════════════

CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 \
           -Iinclude \
           -D_POSIX_C_SOURCE=200809L
LDFLAGS := -lcurl -lsqlite3 -lm

# ── Répertoires ────────────────────────────────────────────
SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BUILD_DIR := build

# ── Sources et objets ──────────────────────────────────────
SRCS := $(SRC_DIR)/user.c       \
        $(SRC_DIR)/session.c    \
        $(SRC_DIR)/interview.c  \
        $(SRC_DIR)/score.c      \
        $(SRC_DIR)/stats.c      \
        $(SRC_DIR)/api.c        \
        $(SRC_DIR)/json_parser.c

# Objets (sans main.c pour les tests)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TARGET       := mockai
TARGET_DEBUG := mockai_debug

# ══════════════════════════════════════════════════════════
# Cibles principales
# ══════════════════════════════════════════════════════════

.PHONY: all debug clean test docs help

all: $(BUILD_DIR) $(TARGET)

debug: CFLAGS += -g -DDEBUG -fsanitize=address,undefined
debug: $(BUILD_DIR) $(TARGET_DEBUG)

# ── Binaire principal ──────────────────────────────────────
$(TARGET): $(OBJS) $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[OK] Binaire : $(TARGET)"

$(TARGET_DEBUG): $(OBJS) $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[OK] Binaire debug : $(TARGET_DEBUG)"

# ── Compilation des objets ─────────────────────────────────
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ══════════════════════════════════════════════════════════
# Tests
# ══════════════════════════════════════════════════════════

# Stub DB minimal pour les tests unitaires (sans SQLite)
TEST_STUB := $(TEST_DIR)/db_stub.c

test: CFLAGS += -g --coverage
test: $(BUILD_DIR) test_user test_core
	@echo "\n[OK] Tous les tests terminés."

test_user: $(OBJS) $(BUILD_DIR)/test_user.o $(BUILD_DIR)/db_stub.o
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@ $(LDFLAGS)
	./$(BUILD_DIR)/$@

test_core: $(OBJS) $(BUILD_DIR)/test_core.o $(BUILD_DIR)/db_stub.o
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@ $(LDFLAGS)
	./$(BUILD_DIR)/$@

$(BUILD_DIR)/test_user.o: $(TEST_DIR)/test_user.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_core.o: $(TEST_DIR)/test_core.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/db_stub.o: $(TEST_DIR)/db_stub.c
	$(CC) $(CFLAGS) -c $< -o $@

# ══════════════════════════════════════════════════════════
# Nettoyage
# ══════════════════════════════════════════════════════════

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET_DEBUG)
	@echo "[OK] Nettoyage terminé."

# ══════════════════════════════════════════════════════════
# Aide
# ══════════════════════════════════════════════════════════

help:
	@echo "Cibles disponibles :"
	@echo "  make          – Compiler le binaire principal"
	@echo "  make debug    – Compiler avec ASan + UBSan"
	@echo "  make test     – Exécuter les tests unitaires"
	@echo "  make clean    – Supprimer les fichiers générés"
