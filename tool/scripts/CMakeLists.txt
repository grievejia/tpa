function (add_script script_name)
	get_filename_component(exec_name ${script_name} NAME_WE)
	file(INSTALL ${script_name}
		DESTINATION ${EXECUTABLE_OUTPUT_PATH}
		RENAME ${exec_name}
		FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endfunction()

add_script(compile-dot.py)
add_script(extract-annotation.py)
add_script(pts-test.py)
add_script(clang-opt.py)