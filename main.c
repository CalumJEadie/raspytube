#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"
#include "GLES/gl.h"
#include "gfxlib.h"
#include "ui.h"
#include "region.h"
#include "mainmenu.h"

//------------------------------------------------------------------------------

typedef struct bufLink
{
    unsigned char buf[BUFSIZ];
    unsigned int count;
    struct bufLink *next;
};

#define AFORMAT_HEIGHT 24
#define AFORMAT_WIDTH 8
extern char * supported_formats[AFORMAT_HEIGHT][AFORMAT_WIDTH];
//------------------------------------------------------------------------------

static bool youtube_search(char * searchStr);
static int create_tcp_socket();


static char *get_ip(char *host);
static char *build_youtube_query(char *host, char *searchStr, int results, int startIndex);
static char *build_file_request(char *host, char *fileName);
//static char * convert_UTF_8(char * src);
static int establish_socket_connection(char * host, int Port);
static void handle_output_jsonc(unsigned int iBracket, unsigned int iBrace, char * key, char * value);
static void parse_buffer_jsonc(char * jsoncContent, bool reset);
static bool youtube_search(char * searchStr);
static void play_video (char * url);
unsigned char *download_file(char * host, char * fileName, unsigned int * fileSize);
unsigned char *find_jpg_start(unsigned char * buf, unsigned int * bufSize);
static void do_search(char * searchStr);
static void do_user_search(char * userStr);
static void do_cur_up();
static void do_cur_down();
static void do_cur_right(char * searchStr);
static void do_cur_left(char * searchStr);

#define USERAGENT "RASPITUBE 1.0"
#define PORT 80
#define HOST "gdata.youtube.com"
#define AUTHOR binarybond007@gmail.com
tMenuState regionMenu;
tMenuState mainMenu;
tMenuState fontMenu;
tMenuState guiMenu;
tMenuState titleFontMenu;
tMenuState formatMenu;

static void do_gui_menu();
static void do_change_audio_dev();
static void do_change_jpeg_dec();
static void do_change_video_player();

#define PICK_SEARCH_STR ((mainMenu.selectedItem == 3 || mainMenu.selectedItem == 4) ? userStr : searchStr)
//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    bcm_host_init();
    memset( state, 0, sizeof( *state ) );
    init_ogl(state);
    init_ui_var();
    //init menus...
    init_small_menu(&mainMenu, "Main Menu:");
    init_big_menu(&regionMenu, "Select region:");
    init_format_menu(&formatMenu);
    init_font_menus();
    //init_small_menu(&fontMenu, "Font menu:");
    //init_small_menu(&titleFontMenu, "Title Font menu:");    
    init_small_menu(&guiMenu, "GUI menu:");
    mainMenu.menuItems = mainMenuItems;
    mainMenu.drawDetail = &main_menu_detail;
    regionMenu.menuItems = &regionMenuItems;
    guiMenu.menuItems = &guiMenuItems;
    guiMenu.drawDetail = &gui_menu_detail;
    set_menu_value(&regionMenu,0);    
    set_font(0);
    set_title_font(4);
    
    initKb();
    clear_output();
    redraw_results(true);
    char searchStr [100] = "";
    char userStr[100] = "";
    char txt[200];
    if(argc > 1)
    {
        youtube_search(argv[1]);
        if(strlen(argv[1]) < sizeof(searchStr))
            strcpy(searchStr, argv[1]);
        else
            show_message("argv[1] too big!!!", 10, ERROR_POINT);
    }
    else
        youtube_search("raspberry+pi");

    int key;
    int result;
    do
    {
        key = toupper(readKb()); //wait for keypress
        switch (key)
        {

        case CUR_UP :
            do_cur_up();
            dumpKb();
            break;

        case CUR_DWN:
            do_cur_down();
            dumpKb();
            break;

        case CUR_R:
            do_cur_right(PICK_SEARCH_STR);
            dumpKb();
            break;

        case CUR_L:
            do_cur_left(PICK_SEARCH_STR);
            dumpKb();
            break;

        case 'H' :
            do_change_audio_dev();
            redraw_results(true);
            dumpKb();
            break;

        case 'T' :
            show_message("TEST-->\n~0|0:~0X~0|1:~1X~0|2:~2X~0|3:~3X~0|4:~4X~0|5:~5X~0|6:~6X~0|7:~7X\n~5press [ESC]", 0xDEADBEEF, ERROR_POINT);
            break;

        case 'M':
            redraw_results(false);
            setBGImage();
            do
            {
                result =  show_menu(&mainMenu);
                if(result > -1)
                {
                    switch(result)
                    {
                    case 1:
                        show_format_menu(&formatMenu);
                        break;
                    case 2:
                        do_search(searchStr);
                        break;
                    case 3:
                    case 4:
                        do_user_search(userStr);
                        break;
                    case 5:
                        show_menu(&regionMenu);
                        break;
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                        numStart = 1;
                        clear_output();
                        clear_screen(true);
                        youtube_search(NULL);
                        if (selected_rec == NULL)
                            show_message("Region search returned 0 results!", 1, ERROR_POINT);
                        break;
                //  case 14:
                //  case 15:
                //      break;
                    case 16:
                        do_gui_menu();
                        break;
                    default:
                        snprintf(txt, sizeof(txt), "item #%d\nkey->%s\ndescription->%s\n~5**UNDER CONSTRUCTION**",
                                result,
                                mainMenuItems[result].key,
                                mainMenuItems[result].description);
                        show_message(txt, 2, ERROR_POINT);
                        break;
                    }
                }
            }
            while (result !=  -1 && 
                   !(result >= 2 && result <= 4) && 
                   !(result >= 6 && result <= 13));
            dumpKb();
            redraw_results(true);
            break;

        case 'R':
            redraw_results(false);
            setBGImage();
            result =  show_menu(&regionMenu);
            if(result > -1)
            {
                snprintf(txt, sizeof(txt), "item #%d\nkey->%s\ndescription->%s",
                        result,
                        regionMenuItems[result].key,
                        regionMenuItems[result].description);
                show_message(txt, 3, ERROR_POINT);
            }
            dumpKb();
            redraw_results(true);
            break;

        case 'X' :
            do_change_jpeg_dec();
            redraw_results(true);
            break;

        case 'F' :
            redraw_results(false);
            setBGImage();
            result = show_format_menu(&formatMenu);
            dumpKb();
            redraw_results(true);
            break;

        case 'P':
            do_change_video_player();
            redraw_results(true);
            dumpKb();
            break;

        case 'I':
            if(selected_rec != NULL)
            {
                do
                {
                    result = show_selection_info(selected_rec);
                    switch(result)
                    {
                    case CUR_L :
                        do_cur_left(PICK_SEARCH_STR);
                        break;
                    case CUR_R:
                        do_cur_right(PICK_SEARCH_STR);
                        break;
                    case CUR_UP :
                        do_cur_up();
                        break;
                    case CUR_DWN:
                        do_cur_down();
                        break;
                    case RTN_KEY:
                        if(selected_rec != NULL && selected_rec->url != NULL)
                            play_video(selected_rec->url);
                        break;
                    }
                    dumpKb();
                }
                while (result != ESC_KEY);
            }
            break;

        case 'N': //new search -> fall through to 'S'
            searchStr[0] = 0x00;

        case 'S' : //modify existing search
            redraw_results(false);
            setBGImage();
            mainMenu.scrollIndex   = 0;
            mainMenu.selectedIndex = mainMenu.selectedItem = 2; 
            do_search(searchStr);
            break;

        case RTN_KEY:
            if(selected_rec != NULL)
            {
                if (selected_rec->url != NULL)
                    play_video(selected_rec->url);
                else
                    show_message("Unable to play:\n\nselected_rec->url==NULL", 4, ERROR_POINT);
            }
            break;

        default :
            break;
        }
    }
    while (key != 'Q' && (key != ESC_KEY || kbHit()));
    clear_output();
    free_ui_var();
    restoreKb();
    exit_func();
    return 0;
}

//------------------------------------------------------------------------------
static void do_gui_menu()
{
    int result;
    int resultFont;
    do
    {
        result = show_menu(&guiMenu);
        switch(result)
        {
            case 0:
                do_change_video_player();
                break;
            case 1:
                do_change_audio_dev();
                break;
            case 2:
                do_change_jpeg_dec();
                break;
            case 3:
                resultFont = show_menu(&fontMenu);     
                if(resultFont != -1)
                     set_font((int) resultFont);
                break;
            case 4:
                resultFont = show_menu(&titleFontMenu);     
                if(resultFont != -1)
                     set_title_font((int) resultFont);
               break;
        }
    }while (result != -1);
}
//------------------------------------------------------------------------------
static void do_cur_up()
{
    if (selected_rec != NULL)
    {
        if(selected_rec->prev != NULL)
            selected_rec = selected_rec->prev;
        else
            selected_rec = last_rec;

        redraw_results(true);
    }
}

//------------------------------------------------------------------------------
static void do_cur_down()
{
    if (selected_rec != NULL)
    {
        if(selected_rec->next != NULL)
            selected_rec = selected_rec->next;
        else
            selected_rec = first_rec;
        redraw_results(true);
    }
}

//------------------------------------------------------------------------------
static void do_cur_right(char * searchStr)
{
    if(numStart < 500)
    {
         numStart += numResults;
         clear_output();
         clear_screen(true);
         youtube_search(searchStr);
    }
}

//------------------------------------------------------------------------------
static void do_cur_left(char * searchStr)
{
    if (numStart > 1)
    {
         numStart -= numResults;
         if(numStart < 1)
             numStart = 1;
         clear_output();
         clear_screen(true);
         youtube_search(searchStr);
    }
}

//------------------------------------------------------------------------------
static void do_change_audio_dev()
{
    if (soundOutput == soHDMI)
       soundOutput = soLOCAL;
    else
       soundOutput = soHDMI;
}    

//------------------------------------------------------------------------------
static void do_change_jpeg_dec()
{
     if (jpegDecoder == jdOMX)
       jpegDecoder = jdLibJpeg;
     else
       jpegDecoder = jdOMX;                                           
}

//------------------------------------------------------------------------------
static void do_change_video_player()
{
    if (videoPlayer == vpOMXPlayer)
       videoPlayer = vpMPlayer;
    else
       videoPlayer = vpOMXPlayer;
}

//------------------------------------------------------------------------------
static void do_search(char * searchStr)
{
    replace_char_str(searchStr, '+', ' ');
    int result = input_string("Search:", searchStr, 50);
    if(result)
    {
        replace_char_str(searchStr, ' ', '+');
        clear_output();
        numStart = 1;
        clear_screen(true);
        youtube_search(searchStr);
        if (selected_rec == NULL)
            show_message("Search returned 0 results!", 5, ERROR_POINT);
    }
    else
        redraw_results(true);
}

//------------------------------------------------------------------------------
static void do_user_search(char * userStr)
{
    char caption[] = "Search ";
    char * prompt = malloc(strlen(mainMenuItems[mainMenu.selectedItem].description) + + strlen(caption)+ 2);
    strcpy(prompt, caption);
    strcat(prompt, mainMenuItems[mainMenu.selectedItem].description);
    strcat(prompt, ":");
    int result = input_string(prompt, userStr, 50);
    free(prompt);
    if(result)
    {
        clear_output();
        numStart = 1;
        clear_screen(true);
        youtube_search(userStr);
        if (selected_rec == NULL)
            show_message("User search returned 0 results!", 6, ERROR_POINT);
    }
    else
        redraw_results(true);
}

//------------------------------------------------------------------------------
static void play_video (char * url)
{
    char * server = NULL;
    char * page = NULL;
    char * freeMe = parse_url(url, &server, &page);
    char url2[4096] = "";
    char request_format[6] = "";
    int status;
    redraw_results(false);
    show_message("Calling youtube_dl...", false, numPointFontMed);
    //show_message(url, false, 20);
    if(server != NULL && page != NULL)
    {
        if (numFormat > 0)
            snprintf(request_format, sizeof(request_format), "-f%s", supported_formats[numFormat+1][0]);
        char youtube_dl_format[] = "youtube-dl %s -g http://%s/%s";
        
        size_t stCommand = strlen(server) +
                           strlen(page) +
                           strlen(request_format) +
                           strlen(youtube_dl_format);
        char * youtube_dl_command   = malloc(stCommand);
        snprintf(youtube_dl_command, stCommand, youtube_dl_format, request_format, server, page);
        free(freeMe);
        FILE * fp = popen(youtube_dl_command, "r");
        unsigned int i = strlen(url2);
        free(youtube_dl_command);
        do
        {
            int c = fgetc(fp);
            if (c == RTN_KEY || c == EOF)
            {
                //trash that Pesky EOF and CR.
            }
            else if(c < 0x80) // Normal char
                url2[i++] = c;
            else // Convert to UTF-8
            {
                url2[i++] = (0xc0|(c & 0xc0)>>6);
                url2[i++] = (0x80|(c & 0x3f));
            }
            if(i > sizeof(url2) - 3)
            {
                show_message("YIKES! popen(youtube-dl) failed. URL TOO LONG!", true, ERROR_POINT);
                return;
            }
        }
        while(!feof(fp));
        url2[i] = 0x00;
        fclose(fp);
        clear_screen(false);
        switch(videoPlayer)
        {
        case vpMPlayer:
            show_message("Calling youtube_dl...~5DONE~0\nStarting Mplayer...", false, numPointFontMed);
            break;
        case vpOMXPlayer:
            show_message("Calling youtube_dl...~5DONE~0\nStarting OMXPlayer...", false, numPointFontMed);
            break;
        }

        int pid = fork();

        if(pid < 0)
        {
            show_message("Fork failed", true, ERROR_POINT);
            exit(errno);
        }

        if (pid == 0)
        {
            int iArgv = 0;
            char * player_argv[6];

            switch(videoPlayer)
            {
            case vpMPlayer:
                player_argv[iArgv++]="/usr/bin/mplayer";
                player_argv[iArgv++]="-fs";
                player_argv[iArgv++]="--";
                break;

            case vpOMXPlayer:
                player_argv[iArgv++]="/usr/bin/omxplayer";

                switch(soundOutput)
                {
                case soHDMI:
                    player_argv[iArgv++]="-ohdmi";
                    break;
                case soLOCAL:
                    player_argv[iArgv++]="-olocal";
                    break;
                default:
                    show_message("Unknown sound output enum", true, ERROR_POINT);
                }

                break;

            default:
                show_message("Unknown video player enum.", true, ERROR_POINT);

            }
            player_argv[iArgv++]=url2;
            player_argv[iArgv++]=NULL;
            execvp(player_argv[0],player_argv);
            exit(200);
        }
        waitpid(pid, &status, 0);
        redraw_results(true);
        dumpKb();
    }
    else
    {
        //yikes server or page is null
        free(freeMe);
        show_message("URL Parser SUX.", true, ERROR_POINT);
    }
}

//------------------------------------------------------------------------------
unsigned char * find_jpg_start(unsigned char * buf, unsigned int* bufSize)
{
    unsigned char * temp;
    for(temp = buf; temp < buf + *bufSize; temp++)
        if(temp[0] == 0xFF && temp[1] == 0xD8)
        {
            *bufSize = *bufSize - (temp - buf);
            return temp;
        }
    return NULL;
}

//------------------------------------------------------------------------------

static void handle_output_jsonc(unsigned int iBracket, unsigned int iBrace, char * key, char * value)
{
    if (iBracket == 3 && strcmp(key, "id") == 0)
    {
        if(first_rec == NULL)
        {
            first_rec = init_result_rec();
            last_rec  = first_rec;
        }
        else
        {
            redraw_results(true);
            struct result_rec * temp_rec = init_result_rec();
            temp_rec->prev = last_rec;
            last_rec->next = temp_rec;
            last_rec = temp_rec;
        }
    }
    char ** pColumn = get_lastrec_column(iBracket, iBrace, key);
    if(pColumn != NULL && *pColumn == NULL )
    {

        *pColumn = malloc(strlen(value)+1);
        strcpy(*pColumn, value);
    }
}

//------------------------------------------------------------------------------
static void parse_buffer_jsonc(char * jsoncContent, bool reset)
{
    char * c = jsoncContent;
    static unsigned int iBracket = 0;
    static unsigned int iBrace   = 0;
    static unsigned int iKey     = 0;
    static unsigned int iValue   = 0;
    static bool quote            = false;
    static bool findkey          = true;
    static char key[128]         = "";
    static char value[1024]      = "";
    static char lastc            = 0x00;

    if(reset)
    {
        quote    = false;
        key[0]   = 0x00;
        value[0] = 0x00;
        iKey     = 0;
        iValue   = 0;
        findkey  = false;
        iBracket = 0;
        iBrace   = 0;
    }

    while(*c != 0x00)
    {
        bool bSkip = false;
        if (*c == '"' && lastc != BSL_KEY )
        {
            quote = !quote;
            bSkip = true;
        }

        if (*c == BSL_KEY && lastc != BSL_KEY)
            bSkip = true;

        if (!quote)
        {
            switch (*c)
            {

            case '{' :
                iBracket++;
                findkey   = true;
                iKey      = 0;
                key[0]    = 0x00;
                bSkip     = true;
                break;

            case '[' :
                findkey   = true;
                iKey      = 0;
                key[0]    = 0x00;
                iBrace++;
                bSkip     = true;
                break;

            case ']' :
                iBrace--;
                bSkip     = true;
                break;

            case '}' :
                handle_output_jsonc(iBracket, iBrace, key, value);
                findkey  = true;
                iKey     = 0;
                key[0]   = 0x00;
                iValue   = 0;
                value[0] = 0x00;
                bSkip    = true;
                iBracket--;
                break;

            case ',' :

                handle_output_jsonc(iBracket, iBrace, key, value);
                findkey  = true;
                iKey     = 0;
                key[0]   = 0x00;
                iValue   = 0;
                value[0] = 0x00;
                bSkip    = true;
                break;

            case '\n' :
                bSkip    = true;
                break;

            case '\r' :
                bSkip    = true;
                break;

            case '\t':
                bSkip    = true;
                break;

            case ':' :
                findkey  = false;
                iValue   = 0;
                value[0] = 0x00;
                bSkip    = true;
                break;
            }
        }

        if(!bSkip)
        {
            if(findkey)
            {
                if (quote && iKey < sizeof(key)-1)
                {
                    key[iKey++] = *c;
                    key[iKey] = 0x00;
                }
            }
            else
            {
                if(iValue < sizeof(value)-1)
                {
                    if(*c != ' ' || quote)
                    {
                        value[iValue++] = *c;
                        value[iValue] = 0x00;
                    }
                }
            }
        }
        lastc = *c;
        c++;
    }
}

//------------------------------------------------------------------------------
int establish_socket_connection(char * host, int Port)
{
    struct sockaddr_in *remote = NULL;
    int sock = 0;
    char *ip = NULL;
    int result;

    sock = create_tcp_socket();
    ip = get_ip(host);
    if(ip == NULL)
    {
        sock = 0;
        goto cleanup;
    }

    remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
    remote->sin_family = AF_INET;
    result = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));

    if(result < 0)
    {
        show_message("Can't set remote->sin_addr.s_addr", true, ERROR_POINT);
        sock = 0;
        goto cleanup;
    }

    else if(result == 0)
    {
        show_message("invalid IP address\n", true, ERROR_POINT);
        sock = 0;
        goto cleanup;
    }

    remote->sin_port = htons(PORT);
    if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0)
    {
        sock = 0;
        goto cleanup;
    }

cleanup:

    if(ip != NULL)
        free(ip);

    if(remote != NULL)
        free(remote);

    return sock;
}

//------------------------------------------------------------------------------
static bool youtube_search(char * searchStr)
{
    char buf[BUFSIZ+1];
    bool reset = true;
    int sock = 0;
    int bytesTrans;
    char * get = NULL;

    sock = establish_socket_connection(HOST, PORT);
    if(sock == 0)
        goto cleanup;

    get = build_youtube_query(HOST, searchStr, numResults, numStart);

    int sent = 0;
    while(sent < strlen(get))
    {
        bytesTrans = send(sock, get+sent, strlen(get)-sent, 0);

        if(bytesTrans == -1)
        {
            show_message("Can't send query", true, ERROR_POINT);
            goto cleanup;
        }
        sent += bytesTrans;
    }
//now it is time to receive the data
    memset(buf, 0, sizeof(buf));
    int htmlstart = 0;
    char * jsoncContent;
    while((bytesTrans = recv(sock, buf, BUFSIZ, 0)) > 0)
    {
        if(htmlstart == 0)
        {
            /* Under certain conditions this will not work.
            * If the \r\n\r\n part is splitted into two messages
            * it will fail to detect the beginning of HTML content
            */
            jsoncContent = strstr(buf, "\r\n\r\n");
            if(jsoncContent != NULL)
            {
                htmlstart = 1;
                jsoncContent += 4;
            }
        }
        else
        {
            jsoncContent = buf;
        }

        if(htmlstart)
        {
            parse_buffer_jsonc(jsoncContent, reset);
            reset=false;
        }
        memset(buf, 0, bytesTrans);
    }

    if(bytesTrans < 0)
    {
        show_message("Error receiving data", true, ERROR_POINT);
        goto cleanup;
    }

    redraw_results(true);
cleanup:

    if (get != NULL)
        free(get);

    if (sock > 0)
        close(sock);

    return 0;
}

//------------------------------------------------------------------------------
unsigned char * download_file(char * host, char * fileName, unsigned int * fileSize)
{
    int sock = 0;
    int bytesTrans;
    char * get = NULL;
    unsigned char * result = NULL;

    sock = establish_socket_connection(host, PORT);
    if(sock == 0)
        goto cleanup;

    get = build_file_request(host, fileName);

    int sent = 0;
    while(sent < strlen(get))
    {
        bytesTrans = send(sock, get+sent, strlen(get)-sent, 0);

        if(bytesTrans == -1)
        {
            show_message("Can't send query", true, ERROR_POINT);
            goto cleanup;
        }
        sent += bytesTrans;
    }

    struct bufLink * saveBuf = NULL;
    struct bufLink * firstBuf = NULL;
    struct bufLink * buf = NULL;
    unsigned int count = 0;
    do
    {
        if(buf != NULL)
            saveBuf= buf;
        buf = malloc(sizeof(struct bufLink));
        if(firstBuf == NULL)
            firstBuf = buf;
        buf->next = NULL;
        if(saveBuf != NULL)
            saveBuf->next = buf;
        buf->count = recv(sock, &buf->buf[0], BUFSIZ, 0);
        count += buf->count;
    }
    while (buf->count > 0);


    if(buf->count < 0)
    {
        show_message("Error receiving data", true, ERROR_POINT);
        goto cleanup;
    }

    result = malloc(count);
    *fileSize = count;

    count = 0;
    buf = firstBuf;
    while (buf != NULL)
    {
        saveBuf = buf;
        memcpy(&result[count], &buf->buf[0], buf->count);
        count += buf->count;
        buf = buf->next;
        free(saveBuf);
    }

cleanup:

    if (get != NULL)
        free(get);

    if (sock > 0)
        close(sock);

    return result;
}
//------------------------------------------------------------------------------

static int create_tcp_socket()
{
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        show_message("Can't create TCP socket", true, ERROR_POINT);
        return 0;
    }
    return sock;
}

//------------------------------------------------------------------------------

static char *get_ip(char *host)
{
    struct hostent *hent;
    int iplen = 15; //XXX.XXX.XXX.XXX
    char *ip = (char *)malloc(iplen+1);
    memset(ip, 0, iplen+1);
    if((hent = gethostbyname(host)) == NULL)
    {
        show_message("Can't get IP", true, ERROR_POINT);
        return NULL;
    }

    if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
    {
        show_message("Can't resolve host", true, ERROR_POINT);
        return NULL;
    }
    return ip;
}

//------------------------------------------------------------------------------

static char *build_youtube_query(char *host, char *searchStr, int results, int startIndex)
{
    #define MAIN_MENU_SPEC_REGION 3
    
    char * query;
    char tempBegin[] = "GET /feeds/api/";
    char * tempEnd = "&max-results=%d&start-index=%d HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
    char * temp; 
    
    char * tempMid = mainMenuItems[
        (mainMenuItems[mainMenu.selectedItem].special == MAIN_MENU_SPEC_REGION)?mainMenu.selectedItem:2].key;
                            
    char country[6] = "";
    size_t stTemp = strlen(tempBegin) +strlen(tempMid) + strlen(tempEnd) + 1;
    temp = malloc(stTemp);
    temp[0] = 0x00;
    strncat(temp, tempBegin, stTemp);
    strncat(temp, tempMid,   stTemp);
    strncat(temp, tempEnd,   stTemp);
    if (mainMenuItems[mainMenu.selectedItem].special == MAIN_MENU_SPEC_REGION)
    {
        if (regionMenu.selectedItem > 0)
        {
            strcat(country, regionMenuItems[regionMenu.selectedItem].key);
            strcat(country, "/");
        }
        searchStr = country;
    }
      
    size_t stQuery = strlen(host) + 
                     strlen(searchStr)+
                     strlen(USERAGENT)+
                     strlen(temp) + 30;
    query = malloc (stQuery);     
    snprintf(query, stQuery, temp, searchStr, results, startIndex, host, USERAGENT);
    free(temp);
    //show_message(query, true, ERROR_POINT);
    return query;
}

//------------------------------------------------------------------------------
static char *build_file_request(char *host, char * fileName)
{
    char *query;
    char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
    if(fileName[0] == '/')
        fileName++;
    size_t stQuery = strlen(host)+strlen(fileName)+strlen(USERAGENT)+strlen(tpl);
    query = (char *)malloc(stQuery);
    snprintf(query, stQuery, tpl, fileName, host, USERAGENT);
    return query;
}
/* now convert on the fly with fgetc
//------------------------------------------------------------------------------
static char * convert_UTF_8(char * src)
{
    int length = strlen(src);
    int i;
    for(i = 0; i < strlen(src); i++)
        if(src[i] >= 80)
            length++;
    char * dst = malloc(length + 1);
    char * tmp = dst;
    for(i =0; i < strlen(src); i++)
    {
        if(src[i]<0x80)
        {
            *tmp = src[i];
            tmp++;
        }
        else
        {
            *tmp = (0xc0|(src[i]&0xc0)>>6);
            tmp++;
            *tmp = (0x80|(src[1]&0x3f));
            tmp++;
        }
    }
    *tmp=0x00;
    return dst;
}
*/
