CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -L$(LIB_DIR) -lapriltag -lm

SRC_DIR = src
INC_DIR = include
UNITY_INC_DIR = unity/src
LIB_DIR = lib
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
UNITY_DIR = unity
UNITY_LIB = $(LIB_DIR)/libunity.a

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJS_WITHOUT_MAIN = $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TESTS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BIN_DIR)/%)

.PHONY: all
all: $(BIN_DIR)/v4l2_camera

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/v4l2_camera $(BIN_DIR)/* $(BUILD_DIR)/*.o

$(BIN_DIR)/v4l2_camera: $(OBJS)
	mkdir -p $(BIN_DIR) && $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR) && $(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

tests: $(TESTS)
	@for test in $(TESTS); do \
		echo Running $$test; \
		./$$test; \
	done


$(BIN_DIR)/%: $(TEST_DIR)/%.c $(UNITY_LIB) $(OBJS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(UNITY_INC_DIR) -o $@ $< $(OBJS_WITHOUT_MAIN) $(LDFLAGS) -lunity

$(UNITY_LIB):
	$(CC) $(CFLAGS) -I$(UNITY_DIR)/src -c $(UNITY_DIR)/src/unity.c -o $(BUILD_DIR)/unity.o
	ar rcs $(UNITY_LIB) $(BUILD_DIR)/unity.o
