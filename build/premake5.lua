-- edibles-raylib build script. Shared logic lives in the ely-arcade-sdk submodule (sdk/).
dofile("../sdk/premake/ely_sdk.lua")

ely.prepare_dirs()
ely.workspace("edibles-raylib")
ely.raylib_project()
ely.sdk_project("../sdk")
ely.app_project("edibles-raylib", "../src", "../sdk")
