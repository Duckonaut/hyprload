# compile with HYPRLAND_HEADERS=<path_to_hl> make all
# make sure that the path above is to the root hl repo directory, NOT src/
# and that you have ran `make protocols` in the hl dir.

PLUGIN_NAME=hyprload

SOURCE_FILES=$(wildcard src/*.cpp)
OBJECT_DIR=obj

INSTALL_PATH=${HOME}/.local/share/hyprload/

COMPILE_FLAGS=-g -fPIC --no-gnu-unique -std=c++23
COMPILE_FLAGS+=`pkg-config --cflags pixman-1 libdrm hyprland`
COMPILE_FLAGS+=-Iinclude

COMPILE_DEFINES=-DWLR_USE_UNSTABLE

ifeq ($(shell whereis -b jq), "jq:")
$(error "jq not found. Please install jq.")
else
BUILT_WITH_NOXWAYLAND=$(shell hyprctl version -j | jq -r '.flags | .[]' | grep 'no xwayland')
ifneq ($(BUILT_WITH_NOXWAYLAND),)
COMPILE_DEFINES+=-DNO_XWAYLAND
endif
endif

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
	@echo $HYPRLAND_COMMIT > "$(INSTALL_PATH)$(PLUGIN_NAME).hlcommit"

check_env:
	@if [ -z "$(HYPRLAND_COMMIT)" ]; then \
		echo 'HYPRLAND_COMMIT not set. Set it to the root Hyprland directory.'; \
		exit 1; \
	fi
	@if pkg-config --exists hyprland; then \
		echo 'Hyprland headers found.'; \
	else \
		echo 'Hyprland headers not available. Run `make pluginenv` in the root Hyprland directory.'; \
		exit 1; \
	fi
	@if [ -z $(BUILT_WITH_NOXWAYLAND) ]; then \
		echo 'Building with XWayland support.'; \
	else \
		echo 'Building without XWayland support.'; \
	fi

$(OBJECT_DIR)/%.o: src/%.cpp
	@mkdir -p $(OBJECT_DIR)
	g++ -c -o $@ $< $(COMPILE_FLAGS) $(COMPILE_DEFINES)

$(PLUGIN_NAME).so: $(addprefix $(OBJECT_DIR)/, $(notdir $(SOURCE_FILES:.cpp=.o)))
	g++ $(LINK_FLAGS) -o $@ $^ $(COMPILE_FLAGS) $(COMPILE_DEFINES)

clean:
	rm -rf $(OBJECT_DIR)
	rm -f $(PLUGIN_NAME).so

clangd:
	echo "$(COMPILE_FLAGS) $(COMPILE_DEFINES)" | \
	sed 's/--no-gnu-unique//g' | \
	sed 's/ -/\n-/g' | \
	sed 's/std=c++23/std=c++2b/g' \
	> compile_flags.txt
