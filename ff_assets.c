#include "ff_assets.h"

#include "ff_consts.h"
#include "ff_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FF_PATH_MAX 512

static char ff_root[FF_PATH_MAX];
static char ff_path[FF_PATH_MAX];
static char ff_fail[4 * FF_PATH_MAX];

static void NoteFailure(const char *path)
{
    size_t n = strlen(ff_fail);

    if (n + strlen(path) + 4 < sizeof ff_fail)
        sprintf(ff_fail + n, "  %s\n", path);
}

static int TryRoot(const char *base, const char *sub)
{
    char probe[FF_PATH_MAX];
    size_t n;

    if (base == NULL)
        return 0;

    snprintf(ff_root, sizeof ff_root, "%s%s", base, sub);
    n = strlen(ff_root);
    if (n > 0 && ff_root[n - 1] != '\\' && ff_root[n - 1] != '/' &&
        n + 1 < sizeof ff_root) {
        ff_root[n]     = '/';
        ff_root[n + 1] = '\0';
    }

    snprintf(probe, sizeof probe, "%sBITMAP/%d.bmp", ff_root, FF_RES_FONT);
    if (Platform_FileExists(probe))
        return 1;

    NoteFailure(probe);
    ff_root[0] = '\0';
    return 0;
}

int Assets_Init(void)
{
    const char *env  = getenv("FFRACE_ASSETS");
    const char *base = Platform_BasePath();

    ff_fail[0] = '\0';
    ff_root[0] = '\0';

    if (TryRoot(env, ""))
        return 1;
    if (TryRoot(base, ""))
        return 1;
    if (TryRoot(base, "assets/"))
        return 1;
    return 0;
}

const char *Assets_Root(void)
{
    return ff_root;
}

const char *Assets_FailureText(void)
{
    return ff_fail;
}

const char *Assets_Bitmap(int res)
{
    snprintf(ff_path, sizeof ff_path, "%sBITMAP/%d.bmp", ff_root, res);
    return ff_path;
}

const char *Assets_Sound(const char *name)
{
    snprintf(ff_path, sizeof ff_path, "%sSounds/%s", ff_root, name);
    return ff_path;
}

/* jumpyball JumpyBall.exe Music_LoadForContext 0x0001db68 */
const char *Assets_Music(const char *name)
{
    snprintf(ff_path, sizeof ff_path, "%sMusics/%s", ff_root, name);
    return ff_path;
}
