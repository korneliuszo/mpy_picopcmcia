include("$(PORT_DIR)/boards/manifest.py")

require("bundle-networking")

require("aioble")

require("aiorepl")

require("sdcard")

module("main.py", base_path="$(BOARD_DIR)/../../src/")
module("picopcmcia.py", base_path="$(BOARD_DIR)/../../src/")
module("bioslayer.py", base_path="$(BOARD_DIR)/../../src/")
module("int13hhandler.py", base_path="$(BOARD_DIR)/../../src/")
module("cga.py", base_path="$(BOARD_DIR)/../../src/")

