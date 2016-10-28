# Build with MSVC 2015

OBJS = winapp.obj
LIBS = user32.lib shell32.lib

winapp.exe: $(OBJS)
	LINK /DEBUG /NOLOGO /INCREMENTAL:NO /OUT:winapp.exe /PDB:winapp1.pdb $(OBJS) $(LIBS)

{}.cpp{}.obj::
	CL /nologo /c /Od /favor:ATOM /EHa /GA /Fdwinapp.pdb /Zi $<

clean:
	del /s /q winapp.exe *.pdb *.obj *.ilk *.tmp
