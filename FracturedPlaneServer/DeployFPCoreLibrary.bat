:: Delete target folder to not leave deleted files there.
del /s /q "..\FracturedPlaneUE\Plugins\FPCoreLibrary\Source\FPCoreLibrary\Public\"

:: Copy entire folder hierarchy from FPCore library includes to Client FPCoreLibrary module includes.
xcopy /s /y "Sources\FPCoreLibrary\PublicIncludes\" "..\FracturedPlaneUE\Plugins\FPCoreLibrary\Source\FPCoreLibrary\Public\"