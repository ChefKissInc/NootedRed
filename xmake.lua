includes(path.join(os.getenv("AppleForgePath") or "../AppleForge", "xmake/rules/AppleForge.lua"))

set_project("NootedRed")
set_version("0.9.0")

AppleForge_ApplyDefaults()
set_allowedarchs("x86_64")
set_defaultarchs("x86_64")

target("NootedRed")
	add_rules("LiluPlugIn")
	set_kind("binary")
	set_languages("cxx23")
	add_cxflags("-Wno-c23-extensions")

	add_files("NootedRed/PenguinWizardry/**.cpp")
	add_files("NootedRed/**.cpp")
	add_includedirs("NootedRed")
	set_values("BundleIdentifier", "org.ChefKiss.NootedRed")
target_end()
