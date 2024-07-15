#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <cstdlib>

void send_no(Display *dpy, XSelectionRequestEvent *sev)
{
    XSelectionEvent ssev;
    char *an;

    an = XGetAtomName(dpy, sev->target);
    printf("Denying request of type '%s'\n", an);
    if (an)
        XFree(an);

    ssev.type = SelectionNotify;
    ssev.requestor = sev->requestor;
    ssev.selection = sev->selection;
    ssev.target = sev->target;
    ssev.property = None;  // signifies "nope"
    ssev.time = sev->time;

    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

void send_utf8(Display *dpy, XSelectionRequestEvent *sev, Atom utf8, unsigned char *utf8Data, int dataLen)
{
    XSelectionEvent ssev;
    time_t now_tm;
    char *now, *an;

    now_tm = time(NULL);
    now = ctime(&now_tm);

    an = XGetAtomName(dpy, sev->property);
    printf("Sending data to window 0x%lx, property '%s'\n", sev->requestor, an);
    if (an)
        XFree(an);

    XChangeProperty(dpy, sev->requestor, sev->property, utf8, 8, PropModeReplace,
                    utf8Data, dataLen);

    ssev.type = SelectionNotify;
    ssev.requestor = sev->requestor;
    ssev.selection = sev->selection;
    ssev.target = sev->target;
    ssev.property = sev->property;
    ssev.time = sev->time;

    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

unsigned char* readFile(FILE* fileToRead, int &fileSize, bool &isOk)
{
    unsigned char *fileData = NULL;
    fseek(fileToRead, 0, SEEK_END);
    fileSize = ftell(fileToRead);
    fseek(fileToRead, 0, SEEK_SET);

    // allocating memory
    fileData = (unsigned char*) malloc(fileSize);
    if(fileData == NULL)
    {
        printf("Can`t allocate %d bytes of memory\n", fileSize);
        isOk = false;
        return fileData;
    }

    int retVal = fread(fileData, sizeof(unsigned char), fileSize, fileToRead);

    if(retVal == fileSize)
    {
        isOk = true;
        return fileData;
    }

    isOk = false;
    return fileData;
}

int main(int argc, char *argv[])
{

    if(argc != 2)
    {
        printf("incorrect output. Must be: file name or utf8 string\n");
        return -1;
    }


    bool useInputedParameter = false;
    unsigned char *fileData = NULL;
    int fileSize = 0;

    FILE* fileStream;
    fileStream = fopen(argv[1], "r");
    if (fileStream == NULL)
    {
        printf("Error can`t open file, use inputed string like utf8 string to klipboard\n");
        useInputedParameter = true;
    }
    else
    {
        bool isOk;
        fileData = readFile(fileStream, fileSize, isOk);
        if(isOk) // successful read
            useInputedParameter = false;
        else
        {
            printf("Error while reading file. Exiting...\n");
            return -1;
        }
    }

    Display *dpy;
    Window owner, root;
    int screen;
    Atom sel, utf8;
    XEvent ev;
    XSelectionRequestEvent *sev;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        fprintf(stderr, "Could not open X display\n");
        return 1;
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    // window to receive messages from other clients.
    owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

    sel = XInternAtom(dpy, "CLIPBOARD", False);
    utf8 = XInternAtom(dpy, "UTF8_STRING", False);

    // claim ownership of the clipboard.
    XSetSelectionOwner(dpy, sel, owner, CurrentTime);

    for (;;)
    {
        XNextEvent(dpy, &ev);
        switch (ev.type)
        {
            case SelectionClear:
                printf("Lost selection ownership\n");
                return 1;
                break;
            case SelectionRequest:
                sev = (XSelectionRequestEvent*)&ev.xselectionrequest;
                printf("Requestor: 0x%lx\n", sev->requestor);
                if (sev->target != utf8 || sev->property == None)
                    send_no(dpy, sev);
                else
                {
                    if(useInputedParameter)
                        send_utf8(dpy, sev, utf8, (unsigned char*)argv[1], strlen(argv[1]));
                    else
                        send_utf8(dpy, sev, utf8, fileData, fileSize);
                }
                break;
        }
    }
}