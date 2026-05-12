include("$(PORT_DIR)/boards/manifest.py")

require("bundle-networking")

require("aioble")

require("sdcard")

module("boot.py", base_path="$(BOARD_DIR)/../../src/")
module("picopcmcia.py", base_path="$(BOARD_DIR)/../../src/")