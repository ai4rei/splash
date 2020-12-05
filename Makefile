LINK = link

CFLAGS = -nologo -W3 -GF -Gz -O1sig -LD -MD $(CFLAGS)
LFLAGS = -nologo -dll -opt:ref -opt:icf user32.lib gdi32.lib $(LFLAGS)
!IF [$(RC) -nologo -? > NUL]==0
RFLAGS = -nologo $(RFLAGS)
!ENDIF

all : splash.dll splash_unicode.dll

splash.dll : dllmain.obj splash.obj splash.res
    $(LINK) $(LFLAGS) -out:$@ $**

splash_unicode.dll : dllmain.obj splash_unicode.obj splash.res
    $(LINK) $(LFLAGS) -out:$@ $**

splash_unicode.obj : splash.c
    $(CC) $(CFLAGS) -DUNICODE -D_UNICODE -Fo$@ -c $**

*.c *.rc : Makefile

.c.obj::
    $(CC) $(CFLAGS) -c $<

.rc.res:
    $(RC) $(RFLAGS) -r $<
