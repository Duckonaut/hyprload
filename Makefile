# compile with HYPRLAND_HEADERS=<path_to_hl> make all
# make sure that the path above is to the root hl repo directory, NOT src/
# and that you have ran `make protocols` in the hl dir.

PLUGIN_NAME=hyprload

SOURCE_FILES=$(wildcard src/*.cpp)
OBJECT_DIR=obj

INSTALL_PATH=${HOME}/.local/share/hyprload/

COMPILE_FLAGS=-g -fPIC --no-gnu-unique -I "/usr/include/pixman-1" -I "/usr/include/libdrm" -I "${HYPRLAND_HEADERS}" -Iinclude -std=c++23
LINK_FLAGS=-shared

.PHONY: install clean clangd

all: check_env $(PLUGIN_NAME).so

install: all
	@mkdir -p $(INSTALL_PATH)plugins/bin
	cp $(PLUGIN_NAME).sh "$(INSTALL_PATH)$(PLUGIN_NAME).sh"
	@if [ -f "$(INSTALL_PATH)$(PLUGIN_NAME).so" ]; then\
		cp $(PLUGIN_NAME).so "$(INSTALL_PATH)$(PLUGIN_NAME).so.update";\
	else\
		cp $(PLUGIN_NAME).so "$(INSTALL_PATH)$(PLUGIN_NAME).so";\
	fi
	@echo $$(git -C $(HYPRLAND_HEADERS) rev-parse HEAD) > "$(INSTALL_PATH)$(PLUGIN_NAME).hlcommit"

check_env:
ifndef HYPRLAND_HEADERS
	$(error HYPRLAND_HEADERS is undefined! Please set it to the path to the root of the configured Hyprland repo)
endif

$(OBJECT_DIR)/%.o: src/%.cpp
	@mkdir -p $(OBJECT_DIR)
	g++ -c -o $@ $< $(COMPILE_FLAGS)

$(PLUGIN_NAME).so: $(addprefix $(OBJECT_DIR)/, $(notdir $(SOURCE_FILES:.cpp=.o)))
	g++ $(LINK_FLAGS) -o $@ $^ $(COMPILE_FLAGS)

clean:
	rm -rf $(OBJECT_DIR)
	rm -f $(PLUGIN_NAME).so

clangd:
	printf "%b" "-I/usr/include/pixman-1\n-I/usr/include/libdrm\n-I${HYPRLAND_HEADERS}\n-Iinclude\n-std=c++2b\n-Wall\n-Wextra" > compile_flags.txt
