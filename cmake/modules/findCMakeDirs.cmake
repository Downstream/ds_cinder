# Recursively finds directories that contain a CMakeLists.txt (ex. examples and tests)
macro( findCMakeDirs RESULT_PATHS BASE_PATH SKIP_PATHS )
	file( GLOB_RECURSE results "${BASE_PATH}/**/CMakeLists.txt" )
	foreach( f ${results} )
		if( EXISTS "${f}" AND NOT IS_DIRECTORY "${f}" )
			ds_log_i( "---- [findCMakeDirs] Found file: ${f}" )

			set( shouldSkip FALSE )
			foreach( skip ${SKIP_PATHS} )
				if( f MATCHES ".*${skip}" )
					ds_log_v( "---- [findCMakeDirs] skipping path: ${f}" )
					set( shouldSkip TRUE )
					break()
				endif()
			endforeach()
			if( shouldSkip )
				continue()
			endif()

			get_filename_component( parentDir ${f} DIRECTORY )
			set( ${RESULT_PATHS} ${${RESULT_PATHS}} ${parentDir} )
		endif()
	endforeach()
endmacro( findCMakeDirs RESULT_PATHS BASE_PATH )