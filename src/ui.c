/* ui.c - ncurses dashboard for the Crescent API.
 *
 * Layout:
 *   +--------------------------------------------------+
 *   | header: handle, stars, verification               |
 *   +----------+-----------------------------------------+
 *   | sidebar  | content (list or detail for the view)   |
 *   | (menu)   |                                          |
 *   +----------+-----------------------------------------+
 *   | footer: keybindings / status / error messages       |
 *   +--------------------------------------------------+
 */
#include "ui.h"
#include "api.h"
#include "json.h"
#include "curses_compat.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
    VIEW_ACCOUNT = 0,
    VIEW_PROJECTS,
    VIEW_PROJECT_DETAIL,
    VIEW_ORDERS,
    VIEW_NOTIFICATIONS,
    VIEW_ANNOUNCEMENTS,
    VIEW_SHOP,
    VIEW_HELP,
    VIEW_COUNT
} view_id;

static const char *MENU_LABELS[] = {
    "Account", "Projects", NULL /* detail has no menu entry */,
    "Orders", "Notifications", "Announcements", "Shop", "Help"
};

/* Menu entries the sidebar actually shows/cycles through (skips the
 * detail view, which is only reached by drilling into a project). */
static const view_id MENU_VIEWS[] = {
    VIEW_ACCOUNT, VIEW_PROJECTS, VIEW_ORDERS, VIEW_NOTIFICATIONS,
    VIEW_ANNOUNCEMENTS, VIEW_SHOP, VIEW_HELP
};
#define MENU_COUNT (int)(sizeof(MENU_VIEWS) / sizeof(MENU_VIEWS[0]))

typedef struct {
    crescent_config *cfg;

    json_value *me;
    json_value *projects;
    json_value *project_detail;
    long project_detail_id;
    json_value *orders;
    json_value *notifications;
    json_value *announcements;
    json_value *shop;

    view_id view;
    view_id prev_view;      /* where Esc/Backspace returns to from detail */
    int sidebar_focus;      /* 1 = sidebar has input focus, 0 = content */
    int menu_sel;           /* index into MENU_VIEWS */
    int list_sel;           /* selected row within the current content list */
    int list_scroll;        /* first visible row */

    char status_msg[256];
    int status_is_error;

    int running;
} app;

/* ---- small utilities --------------------------------------------- */

static void set_status(app *a, int is_error, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
static void set_status(app *a, int is_error, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(a->status_msg, sizeof(a->status_msg), fmt, ap);
    va_end(ap);
    a->status_is_error = is_error;
}

/* Reformats an ISO-8601 timestamp ("2026-09-10T16:30:00Z") down to
 * "2026-09-10 16:30" for compact display. Falls back to the raw string
 * for anything that doesn't look like the expected shape. */
static const char *fmt_time(const char *iso, char *out, size_t outlen) {
    if (!iso) { snprintf(out, outlen, "-"); return out; }
    int y, mo, d, h, mi;
    if (sscanf(iso, "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) == 5) {
        snprintf(out, outlen, "%04d-%02d-%02d %02d:%02d", y, mo, d, h, mi);
        return out;
    }
    snprintf(out, outlen, "%s", iso);
    return out;
}

static void truncate_into(char *dst, size_t dstlen, const char *src) {
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= dstlen) n = dstlen - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ---- color / status styling ---------------------------------------- */

enum {
    CP_HEADER = 1,
    CP_SIDEBAR_SEL,
    CP_ROW_SEL,
    CP_GOOD,
    CP_WARN,
    CP_BAD,
    CP_INFO,
    CP_MUTED,
    CP_ERRORBAR
};

static void init_colors(void) {
    start_color();
    use_default_colors();
    init_pair(CP_HEADER, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_SIDEBAR_SEL, COLOR_BLACK, COLOR_CYAN);
    init_pair(CP_ROW_SEL, COLOR_BLACK, COLOR_WHITE);
    init_pair(CP_GOOD, COLOR_GREEN, -1);
    init_pair(CP_WARN, COLOR_YELLOW, -1);
    init_pair(CP_BAD, COLOR_RED, -1);
    init_pair(CP_INFO, COLOR_MAGENTA, -1);
    init_pair(CP_MUTED, COLOR_CYAN, -1);
    init_pair(CP_ERRORBAR, COLOR_WHITE, COLOR_RED);
}

/* Maps the many status enums used across the API to a color pair. */
static int status_pair(const char *status) {
    if (!status) return CP_MUTED;
    if (!strcmp(status, "approved") || !strcmp(status, "fulfilled") ||
        !strcmp(status, "verified") || !strcmp(status, "order_done") ||
        !strcmp(status, "ship_approved") || !strcmp(status, "order_grant_sent"))
        return CP_GOOD;
    if (!strcmp(status, "under_review") || !strcmp(status, "pending") ||
        !strcmp(status, "on_hold") || !strcmp(status, "awaiting_verification"))
        return CP_WARN;
    if (!strcmp(status, "rejected") || !strcmp(status, "cancelled") ||
        !strcmp(status, "expired") || !strcmp(status, "ineligible") ||
        !strcmp(status, "forbidden"))
        return CP_BAD;
    if (!strcmp(status, "needs_changes") || !strcmp(status, "ship_needs_changes"))
        return CP_INFO;
    if (!strcmp(status, "draft"))
        return CP_MUTED;
    return CP_MUTED;
}

/* ---- data fetching --------------------------------------------------- */

static void free_view_caches(app *a) {
    json_free(a->me); a->me = NULL;
    json_free(a->projects); a->projects = NULL;
    json_free(a->project_detail); a->project_detail = NULL;
    json_free(a->orders); a->orders = NULL;
    json_free(a->notifications); a->notifications = NULL;
    json_free(a->announcements); a->announcements = NULL;
    json_free(a->shop); a->shop = NULL;
}

static const char *api_err_hint(api_status s) {
    switch (s) {
        case API_ERR_UNAUTHORIZED: return "check your API key (account menu -> API keys on Crescent)";
        case API_ERR_FORBIDDEN: return "this account is banned";
        case API_ERR_RATE_LIMIT: return "slow down: 60 requests/min per key";
        case API_ERR_DISABLED: return "Crescent's API is temporarily switched off";
        case API_ERR_TRANSPORT: return "check your network connection";
        default: return "";
    }
}

/* Fetches whatever the active view needs and caches it. Safe to call
 * repeatedly (e.g. on manual refresh); frees any prior cache for that
 * view first. */
static void load_view(app *a, view_id v) {
    const char *key = a->cfg->api_key;
    api_result r = {0};

    switch (v) {
        case VIEW_ACCOUNT:
            json_free(a->me); a->me = NULL;
            r = api_get_me(key);
            if (r.status == API_OK) a->me = r.data, r.data = NULL;
            break;
        case VIEW_PROJECTS:
            json_free(a->projects); a->projects = NULL;
            r = api_list_projects(key);
            if (r.status == API_OK){
                a->projects =json_take(r.data,"projects");
            }
            a->list_sel = 0; a->list_scroll = 0;
            break;
        case VIEW_ORDERS:
            json_free(a->orders); a->orders = NULL;
            r = api_list_orders(key);
            if (r.status == API_OK) a->orders = r.data, r.data = NULL;
            a->list_sel = 0; a->list_scroll = 0;
            break;
        case VIEW_NOTIFICATIONS:
            json_free(a->notifications); a->notifications = NULL;
            r = api_list_notifications(key, 50);
            if (r.status == API_OK) a->notifications = r.data, r.data = NULL;
            a->list_sel = 0; a->list_scroll = 0;
            break;
        case VIEW_ANNOUNCEMENTS:
            json_free(a->announcements); a->announcements = NULL;
            r = api_list_announcements(key);
            if (r.status == API_OK){
                a->announcements = json_take(r.data,"announcements");
            }
            a->list_sel = 0; a->list_scroll = 0;
            break;
        case VIEW_SHOP:
            json_free(a->shop); a->shop = NULL;
            r = api_get_shop_items();
            if (r.status == API_OK) a->shop = r.data, r.data = NULL;
            a->list_sel = 0; a->list_scroll = 0;
            break;
        default:
            return;
    }

    if (r.status == API_OK) {
        set_status(a, 0, "loaded.");
    } else {
        const char *hint = api_err_hint(r.status);
        if (hint && *hint)
            set_status(a, 1, "%s (%s)", r.message, hint);
        else
            set_status(a, 1, "%s", r.message);
    }
    api_result_free(&r);
}

static void load_project_detail(app *a, long id) {
    json_free(a->project_detail);
    a->project_detail = NULL;
    api_result r = api_get_project(a->cfg->api_key, id);
    if (r.status == API_OK) {
        a->project_detail = r.data;
        r.data = NULL;
        a->project_detail_id = id;
        set_status(a, 0, "loaded.");
    } else {
        set_status(a, 1, "%s (%s)", r.message, api_err_hint(r.status));
    }
    api_result_free(&r);
}

/* ---- drawing ----------------------------------------------------------- */

#define SIDEBAR_W 20

static void draw_header(app *a, int width) {
    attron(COLOR_PAIR(CP_HEADER));
    for (int x = 0; x < width; x++) mvaddch(0, x, ' ');
    char left[160];
    if (a->me) {
        const char *handle = json_get_string(a->me, "handle", "?");
        long stars = json_get_int(a->me, "stars", 0);
        const char *verif = json_get_string(a->me, "verificationStatus", "?");
        snprintf(left, sizeof(left), " Crescent  |  %s  |  %ld stars  |  %s",
                 handle, stars, verif);
    } else {
        snprintf(left, sizeof(left), " Crescent");
    }
    mvprintw(0, 0, "%.*s", width, left);
    char right[32];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(right, sizeof(right), "%Y-%m-%d %H:%M", tm);
    int rlen = (int)strlen(right);
    if (width - rlen - 1 > (int)strlen(left))
        mvprintw(0, width - rlen - 1, "%s", right);
    attroff(COLOR_PAIR(CP_HEADER));
}

static void draw_sidebar(app *a, int top, int height) {
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = top + i;
        if (y >= top + height) break;
        int selected = (i == a->menu_sel);
        int focused = selected && a->sidebar_focus;
        if (focused) attron(COLOR_PAIR(CP_SIDEBAR_SEL) | A_BOLD);
        else if (selected) attron(A_BOLD);
        mvprintw(y, 0, " %-*s", SIDEBAR_W - 1, MENU_LABELS[MENU_VIEWS[i]]);
        if (focused) attroff(COLOR_PAIR(CP_SIDEBAR_SEL) | A_BOLD);
        else if (selected) attroff(A_BOLD);
    }
    mvprintw(top + MENU_COUNT + 1, 0, " %-*s", SIDEBAR_W - 1, "Quit (q)");
    for (int y = top; y < top + height; y++)
        mvaddch(y, SIDEBAR_W, ACS_VLINE);
}

static void draw_footer(app *a, int row, int width) {
    move(row, 0);
    clrtoeol();
    if (a->status_is_error) {
        attron(COLOR_PAIR(CP_ERRORBAR));
        mvprintw(row, 0, " %.*s", width - 1, a->status_msg);
        attroff(COLOR_PAIR(CP_ERRORBAR));
    } else {
        const char *hint = a->sidebar_focus
            ? "up/down: menu   enter/l: open   q: quit   ?: help"
            : "up/down: move   enter: open   esc/h: back   r: refresh   q: quit   ?: help";
        mvprintw(row, 0, " %.*s", width - 1, hint);
        if (a->status_msg[0])
            mvprintw(row, width - (int)strlen(a->status_msg) - 2, "%s", a->status_msg);
    }
}

/* Draws one selectable row of content, handling the highlight + wrapping
 * to the pane width. */
static void content_row(int y, int x, int width, int is_selected, int pair,
                         const char *fmt, ...) __attribute__((format(printf, 6, 7)));
static void content_row(int y, int x, int width, int is_selected, int pair,
                         const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    move(y, x);
    clrtoeol();
    if (is_selected) attron(COLOR_PAIR(CP_ROW_SEL));
    else if (pair) attron(COLOR_PAIR(pair));
    mvprintw(y, x, "%.*s", width, buf);
    if (is_selected) attroff(COLOR_PAIR(CP_ROW_SEL));
    else if (pair) attroff(COLOR_PAIR(pair));
}

static void draw_account(app *a, int top, int left, int width, int height) {
    (void)height;
    if (!a->me) {
        mvprintw(top, left, "(no data - press r to load)");
        return;
    }
    char tbuf[32];
    int y = top;
    mvprintw(y++, left, "Handle:        %s", json_get_string(a->me, "handle", "-"));
    mvprintw(y++, left, "Slack ID:      %s", json_get_string(a->me, "slackId", "-"));
    mvprintw(y++, left, "Stars:         %ld", json_get_int(a->me, "stars", 0));
    const char *verif = json_get_string(a->me, "verificationStatus", "-");
    attron(COLOR_PAIR(status_pair(verif)));
    mvprintw(y++, left, "Verification:  %s", verif);
    attroff(COLOR_PAIR(status_pair(verif)));
    mvprintw(y++, left, "Joined:        %s", fmt_time(json_get_string(a->me, "joinedAt", NULL), tbuf, sizeof(tbuf)));
    y++;
    mvprintw(y++, left, "Avatar URL:");
    mvprintw(y++, left, "  %.*s", width - 2, json_get_string(a->me, "avatarUrl", "-"));
}

static void draw_list_scrollbar_note(int row, int col, int shown, int total) {
    if (total > shown)
        mvprintw(row, col, "(%d/%d, scroll with up/down)", shown, total);
}

static void draw_projects_list(app *a, int top, int left, int width, int height) {
    json_value *list = a->projects;
    size_t n = json_array_count(list);
    if (!list) { mvprintw(top, left, "(no data - press r to load)"); return; }
    if (n == 0) { mvprintw(top, left, "No projects yet."); return; }

    int rows = height - 1;
    if (a->list_sel >= (int)n) a->list_sel = (int)n - 1;
    if (a->list_sel < a->list_scroll) a->list_scroll = a->list_sel;
    if (a->list_sel >= a->list_scroll + rows) a->list_scroll = a->list_sel - rows + 1;

    for (int i = 0; i < rows; i++) {
        size_t idx = (size_t)(a->list_scroll + i);
        if (idx >= n) break;
        json_value *p = json_array_at(list, idx);
        const char *title = json_get_string(p, "title", "(untitled)");
        const char *status = json_get_string(p, "status", "-");
        const char *card = NULL;
        json_value *card_obj = json_get(p, "card");
        if (card_obj) card = json_get_string(card_obj, "name", NULL);
        long tracked = json_get_int(p, "trackedSeconds", 0);
        int sel = (idx == (size_t)a->list_sel) && !a->sidebar_focus;
        content_row(top + i, left, width, sel, status_pair(status),
                    "%-9s  %-28.28s  %-14s  %dh%02dm",
                    status, title, card ? card : "-",
                    (int)(tracked / 3600), (int)((tracked % 3600) / 60));
    }
    draw_list_scrollbar_note(top + rows, left, rows < (int)n ? rows : (int)n, (int)n);
}

static void draw_project_detail(app *a, int top, int left, int width, int height) {
    (void)height;
    json_value *p = a->project_detail;
    if (!p) { mvprintw(top, left, "(loading...)"); return; }
    char tbuf[32];
    int y = top;
    const char *status = json_get_string(p, "status", "-");
    mvprintw(y, left, "Title:      %s", json_get_string(p, "title", "-"));
    y++;
    const char *desc = json_get_string(p, "description", NULL);
    if (desc) {
        mvprintw(y++, left, "Description:");
        mvprintw(y++, left, "  %.*s", width - 2, desc);
    }
    attron(COLOR_PAIR(status_pair(status)));
    mvprintw(y, left, "Status:     %s", status);
    attroff(COLOR_PAIR(status_pair(status)));
    y++;
    json_value *card = json_get(p, "card");
    if (card) {
        mvprintw(y++, left, "Card:       %s (x%.2f%s)", json_get_string(card, "name", "-"),
                 json_get_double(card, "multiplier", 1.0),
                 json_get_bool(card, "wildcard", 0) ? ", wildcard" : "");
        const char *req = json_get_string(card, "requirement", NULL);
        if (req)
            mvprintw(y++, left, "  requires: %.*s", width - 12, req);
        const char *guide = json_get_string(card, "guideUrl", NULL);
        if (guide)
            mvprintw(y++, left, "  guide:    %.*s", width - 12, guide);
    }
    mvprintw(y++, left, "Repo:       %.*s", width - 12, json_get_string(p, "repoUrl", "-"));
    mvprintw(y++, left, "Demo:       %.*s", width - 12, json_get_string(p, "demoUrl", "-"));
    mvprintw(y++, left, "Page:       %.*s", width - 12, json_get_string(p, "url", "-"));
    long tracked = json_get_int(p, "trackedSeconds", 0);
    mvprintw(y++, left, "Tracked:    %dh %02dm", (int)(tracked / 3600), (int)((tracked % 3600) / 60));
    mvprintw(y++, left, "Created:    %s", fmt_time(json_get_string(p, "createdAt", NULL), tbuf, sizeof(tbuf)));
    mvprintw(y++, left, "Approved:   %s", fmt_time(json_get_string(p, "approvedAt", NULL), tbuf, sizeof(tbuf)));

    const char *feedback = json_get_string(p, "feedback", NULL);
    if (feedback) {
        y++;
        attron(COLOR_PAIR(CP_INFO));
        mvprintw(y++, left, "Reviewer feedback:");
        mvprintw(y++, left, "  %.*s", width - 2, feedback);
        attroff(COLOR_PAIR(CP_INFO));
    }

    json_value *ships = json_get(p, "ships");
    size_t sc = json_array_count(ships);
    y += 1;
    mvprintw(y++, left, "Ships (%zu):", sc);
    for (size_t i = 0; i < sc && y < top + 40; i++) {
        json_value *s = json_array_at(ships, i);
        const char *sstatus = json_get_string(s, "status", "-");
        int stars_null = json_is_null(s, "stars");
        attron(COLOR_PAIR(status_pair(sstatus)));
        mvprintw(y, left + 2, "%-9s", sstatus);
        attroff(COLOR_PAIR(status_pair(sstatus)));
        if (!stars_null)
            printw("  %4ld stars ", json_get_int(s, "stars", 0));
        else
            printw("  %11s ", "-");
        printw(" submitted %s", json_get_string(s, "submittedAt", "-"));
        y++;
        const char *chg = json_get_string(s, "changeDescription", NULL);
        if (chg) { mvprintw(y, left + 4, "%.*s", width - 6, chg); y++; }
    }
    mvprintw(top - 1, left, "esc/backspace: back to project list");
}

static void draw_orders(app *a, int top, int left, int width, int height) {
    json_value *list = a->orders;
    size_t n = json_array_count(list);
    if (!list) { mvprintw(top, left, "(no data - press r to load)"); return; }
    if (n == 0) { mvprintw(top, left, "No orders yet."); return; }

    int rows = height - 1;
    if (a->list_sel >= (int)n) a->list_sel = (int)n - 1;
    if (a->list_sel < a->list_scroll) a->list_scroll = a->list_sel;
    if (a->list_sel >= a->list_scroll + rows) a->list_scroll = a->list_sel - rows + 1;

    for (int i = 0; i < rows; i++) {
        size_t idx = (size_t)(a->list_scroll + i);
        if (idx >= n) break;
        json_value *o = json_array_at(list, idx);
        const char *status = json_get_string(o, "status", "-");
        const char *label = json_get_string(o, "label", status);
        json_value *item = json_get(o, "item");
        const char *name = item ? json_get_string(item, "name", "?") : "?";
        long qty = json_get_int(o, "quantity", 1);
        long total = json_get_int(o, "totalPrice", 0);
        int sel = (idx == (size_t)a->list_sel) && !a->sidebar_focus;
        content_row(top + i, left, width, sel, status_pair(status),
                    "%-10s  %-22.22s  x%-3ld  %5ld stars  (%s)",
                    status, name, qty, total, label);
    }
    draw_list_scrollbar_note(top + rows, left, rows < (int)n ? rows : (int)n, (int)n);

    if (!a->sidebar_focus && n > 0 && a->list_sel < (int)n) {
        json_value *o = json_array_at(list, (size_t)a->list_sel);
        const char *carrier = json_get_string(o, "trackingCarrier", NULL);
        const char *turl = json_get_string(o, "trackingUrl", NULL);
        long id = json_get_int(o, "id", -1);
        long unit = json_get_int(o, "unitPrice", 0);
        char created[32], fulfilled[32];
        fmt_time(json_get_string(o, "createdAt", NULL), created, sizeof(created));
        fmt_time(json_get_string(o, "fulfilledAt", NULL), fulfilled, sizeof(fulfilled));
        int y = top + rows + 1;
        mvprintw(y++, left, "Order #%ld  |  %ld stars each  |  placed %s  |  fulfilled %s",
                 id, unit, created,
                 json_is_null(o, "fulfilledAt") ? "-" : fulfilled);
        if (carrier) mvprintw(y++, left, "Tracking: %s  %.*s", carrier, width - 12, turl ? turl : "");
    }
}

static void draw_notifications(app *a, int top, int left, int width, int height) {
    json_value *list = a->notifications;
    size_t n = json_array_count(list);
    if (!list) { mvprintw(top, left, "(no data - press r to load)"); return; }
    if (n == 0) { mvprintw(top, left, "No notifications."); return; }

    int rows = height - 1;
    if (a->list_sel >= (int)n) a->list_sel = (int)n - 1;
    if (a->list_sel < a->list_scroll) a->list_scroll = a->list_sel;
    if (a->list_sel >= a->list_scroll + rows) a->list_scroll = a->list_sel - rows + 1;

    for (int i = 0; i < rows; i++) {
        size_t idx = (size_t)(a->list_scroll + i);
        if (idx >= n) break;
        json_value *note = json_array_at(list, idx);
        int read = json_get_bool(note, "read", 1);
        const char *title = json_get_string(note, "title", "-");
        const char *kind = json_get_string(note, "kind", "-");
        char tbuf[32];
        fmt_time(json_get_string(note, "createdAt", NULL), tbuf, sizeof(tbuf));
        int sel = (idx == (size_t)a->list_sel) && !a->sidebar_focus;
        content_row(top + i, left, width, sel, status_pair(kind),
                    "%s %-16s  %-30.30s  %s",
                    read ? " " : "*", kind, title, tbuf);
    }
    draw_list_scrollbar_note(top + rows, left, rows < (int)n ? rows : (int)n, (int)n);

    if (!a->sidebar_focus && n > 0 && a->list_sel < (int)n) {
        json_value *note = json_array_at(list, (size_t)a->list_sel);
        int y = top + rows + 1;
        mvprintw(y++, left, "%.*s", width, json_get_string(note, "body", ""));
        const char *url = json_get_string(note, "url", NULL);
        if (url) mvprintw(y++, left, "%.*s", width, url);
    }
}

static void draw_announcements(app *a, int top, int left, int width, int height) {
    json_value *list = a->announcements;
    size_t n = json_array_count(list);
    if (!list) { mvprintw(top, left, "(no data - press r to load)"); return; }
    if (n == 0) { mvprintw(top, left, "No announcements."); return; }

    int y = top;
    for (size_t i = 0; i < n && y < top + height - 1; i++) {
        json_value *an = json_array_at(list, i);
        json_value *author = json_get(an, "author");
        const char *name = author ? json_get_string(author, "name", "?") : "?";
        char tbuf[32];
        fmt_time(json_get_string(an, "postedAt", NULL), tbuf, sizeof(tbuf));
        int sel = ((int)i == a->list_sel) && !a->sidebar_focus;
        if (sel) attron(COLOR_PAIR(CP_ROW_SEL));
        attron(A_BOLD);
        mvprintw(y++, left, "%s  -  %s", name, tbuf);
        attroff(A_BOLD);
        const char *title = json_get_string(an, "title", NULL);
        if (title) {
            attron(A_BOLD);
            mvprintw(y++, left, "  %.*s", width - 2, title);
            attroff(A_BOLD);
        }
        const char *text = json_get_string(an, "text", "");
        char line[300];
        truncate_into(line, sizeof(line), text);
        mvprintw(y++, left, "  %.*s%s", width - 4, line,
                 json_get_bool(an, "truncated", 0) ? " [...]" : "");
        if (sel) {
            const char *url = json_get_string(an, "url", NULL);
            if (url) mvprintw(y++, left, "  %.*s", width - 2, url);
            attroff(COLOR_PAIR(CP_ROW_SEL));
        }
        y++;
    }
}

static void draw_shop(app *a, int top, int left, int width, int height) {
    json_value *feed = a->shop;
    if (!feed) { mvprintw(top, left, "(no data - press r to load)"); return; }
    json_value *items = json_get(feed, "items");
    size_t n = json_array_count(items);
    const char *region = a->cfg->region;
    double dpu = json_get_double(feed, "dollarsPerUnit", 0.0);

    mvprintw(top, left, "Region: %-4s (press R to cycle)   currency: %s (1 = $%.2f)",
             region, json_get_string(feed, "currency", "stars"), dpu);
    int list_top = top + 2;
    int rows = height - 3;
    if (a->list_sel >= (int)n) a->list_sel = n ? (int)n - 1 : 0;
    if (a->list_sel < a->list_scroll) a->list_scroll = a->list_sel;
    if (a->list_sel >= a->list_scroll + rows) a->list_scroll = a->list_sel - rows + 1;

    for (int i = 0; i < rows; i++) {
        size_t idx = (size_t)(a->list_scroll + i);
        if (idx >= n) break;
        json_value *it = json_array_at(items, idx);
        const char *name = json_get_string(it, "name", "?");
        const char *type = json_get_string(it, "type", "-");
        int stock = json_get_bool(it, "inStock", 1);
        int one_per = json_get_bool(it, "onePerUser", 0);
        json_value *prices = json_get(it, "prices");
        long price = prices ? json_get_int(prices, region, -1) : -1;
        char pricebuf[32];
        if (price >= 0) snprintf(pricebuf, sizeof(pricebuf), "%ld (~$%.0f)", price, price * dpu);
        else snprintf(pricebuf, sizeof(pricebuf), "n/a");
        int sel = (idx == (size_t)a->list_sel) && !a->sidebar_focus;
        content_row(list_top + i, left, width, sel, stock ? 0 : CP_MUTED,
                    "%-26.26s  %-9s  %-14s  %s%s", name, type, pricebuf,
                    stock ? "in stock" : "out of stock",
                    one_per ? "  (1/user)" : "");
    }
    draw_list_scrollbar_note(list_top + rows, left, rows < (int)n ? rows : (int)n, (int)n);

    if (!a->sidebar_focus && n > 0 && a->list_sel < (int)n) {
        json_value *it = json_array_at(items, (size_t)a->list_sel);
        const char *desc = json_get_string(it, "description", "");
        int y = list_top + rows + 1;
        mvprintw(y++, left, "%.*s", width, desc);
        const char *url = json_get_string(it, "url", NULL);
        if (url) mvprintw(y++, left, "%.*s", width, url);
        if (!json_is_null(it, "grantRules"))
            mvprintw(y++, left, "(this item has grant rules attached)");
    }
}

static void draw_help(int top, int left, int width, int height) {
    (void)width; (void)height;
    const char *lines[] = {
        "Crescent TUI - keybindings",
        "",
        "  Up/Down or j/k     move selection (menu or list)",
        "  Enter / l          open: drill into a project's detail,",
        "                     or move focus from menu into content",
        "  Esc / Backspace / h  go back: to the list, or to the menu",
        "  r                  refresh the current view",
        "  R                  (Shop view) cycle price region",
        "  ?                  toggle this help",
        "  q                  quit",
        "",
        "This dashboard only ever shows data belonging to the account",
        "that owns the API key you configured - Crescent's API has no",
        "endpoint to see or manage other participants.",
    };
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
        mvprintw(top + (int)i, left, "%s", lines[i]);
}

static void draw_content(app *a, int top, int left, int width, int height) {
    switch (a->view) {
        case VIEW_ACCOUNT: draw_account(a, top, left, width, height); break;
        case VIEW_PROJECTS: draw_projects_list(a, top, left, width, height); break;
        case VIEW_PROJECT_DETAIL: draw_project_detail(a, top, left, width, height); break;
        case VIEW_ORDERS: draw_orders(a, top, left, width, height); break;
        case VIEW_NOTIFICATIONS: draw_notifications(a, top, left, width, height); break;
        case VIEW_ANNOUNCEMENTS: draw_announcements(a, top, left, width, height); break;
        case VIEW_SHOP: draw_shop(a, top, left, width, height); break;
        case VIEW_HELP: draw_help(top, left, width, height); break;
        default: break;
    }
}

static void render(app *a) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    erase();

    if (rows < 12 || cols < SIDEBAR_W + 20) {
        const char *msg = "Terminal too small. Resize the terminal.";
        int y = rows > 0 ? rows / 2 : 0;
        int x = 0;

        if (cols > (int)strlen(msg))
            x = (cols - (int)strlen(msg)) / 2;

        if (rows > 0 && cols > 0)
            mvprintw(y, x, "%.*s", cols, msg);

        refresh();
        return;
    }

    draw_header(a, cols);

    int content_top = a->view == VIEW_PROJECT_DETAIL ? 3 : 2;
    int content_left = SIDEBAR_W + 2;
    int content_width = cols - SIDEBAR_W - 3;
    int content_height = rows - content_top - 1;

    draw_sidebar(a, 2, rows - 3);
    draw_content(a, content_top, content_left,
                 content_width, content_height);
    draw_footer(a, rows - 1, cols);

    refresh();
}
/* ---- first-run API key prompt ---------------------------------------- */

/* Simple centered modal: prompts for a line of text, drawing '*' instead
 * of the real characters when `mask` is set. Returns 1 with `buf` filled
 * on Enter, 0 if the user cancels with Esc. */
static int prompt_line(const char *title, const char *prompt, char *buf, size_t bufsize, int mask) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int w = cols > 70 ? 64 : cols - 6;
    int h = 6;

    if (rows < h || w < 20) {
        return 0;
    }

    int y0 = (rows - h) / 2;
    int x0 = (cols - w) / 2;

    WINDOW *win = newwin(h, w, y0, x0);
    if (!win)
        return 0;
    keypad(win, TRUE);
    size_t len = strlen(buf);
    curs_set(1);

    for (;;) {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 0, 2, " %s ", title);
        mvwprintw(win, 1, 2, "%.*s", w - 4, prompt);
        mvwaddch(win, 3, 2, '>');
        waddch(win, ' ');
        if (mask) {
            for (size_t i = 0; i < len; i++) waddch(win, '*');
        } else {
            waddnstr(win, buf, (int)len);
        }
        mvwprintw(win, 4, 2, "[Enter] confirm   [Esc] cancel");
        wmove(win, 3, 4 + (int)len);
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == '\n' || ch == KEY_ENTER) {
            buf[len] = '\0';
            curs_set(0);
            delwin(win);
            return 1;
        } else if (ch == 27) { /* Esc */
            curs_set(0);
            delwin(win);
            return 0;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (len > 0) len--;
        } else if (isprint(ch) && len + 1 < bufsize) {
            buf[len++] = (char)ch;
        }
    }
}

static int confirm_dialog(const char *question) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int w = (int)strlen(question) + 12;

    if (w > cols - 4)
        w = cols - 4;

    int h = 4;

    if (rows < h || w < 20)
        return 0;

    int y0 = (rows - h) / 2;
    int x0 = (cols - w) / 2;

    WINDOW *win = newwin(h, w, y0, x0);
    if (!win)
        return 0;

    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "%.*s", w - 4, question);
    mvwprintw(win, 2, 2, "[y]es   [n]o");

    wrefresh(win);

    int result = 0;

    for (;;) {
        int ch = wgetch(win);

        if (ch == 'y' || ch == 'Y') {
            result = 1;
            break;
        }

        if (ch == 'n' || ch == 'N' || ch == 27) {
            result = 0;
            break;
        }
    }

    delwin(win);
    return result;
}

static void ensure_api_key(crescent_config *cfg) {
    if (cfg->api_key[0]) return;
    char key[128] = {0};
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows; (void)cols;
    if (prompt_line("Crescent API key needed",
                     "Paste a key from Crescent -> account menu -> API keys:",
                     key, sizeof(key), 1)) {
        strncpy(cfg->api_key, key, sizeof(cfg->api_key) - 1);
        cfg->api_key[sizeof(cfg->api_key) - 1] = '\0';
        if (confirm_dialog("Save this key to ~/.config/crescent-tui/config (mode 600)?")) {
            if (config_save(cfg) != 0) {
                fprintf(stderr, "Failed to save configuration.\n");
            } else {
                fprintf(stderr, "API key saved.\n");
            }
        }
    }
}

/* ---- main loop --------------------------------------------------------- */

/* Falls back to this list only if the shop feed hasn't loaded yet (e.g.
 * cycling region before the first fetch completes); once `a->shop` is
 * present we cycle through its own `regions` array instead, so we track
 * whatever the API actually serves rather than a guessed list. */
static const char *FALLBACK_REGIONS[] = {"US", "EU", "UK", "IN", "CA", "AU", "XX"};
#define FALLBACK_REGIONS_COUNT (int)(sizeof(FALLBACK_REGIONS) / sizeof(FALLBACK_REGIONS[0]))

static void ensure_loaded(app *a, view_id v) {
    json_value **cache = NULL;
    switch (v) {
        case VIEW_ACCOUNT: cache = &a->me; break;
        case VIEW_PROJECTS: cache = &a->projects; break;
        case VIEW_ORDERS: cache = &a->orders; break;
        case VIEW_NOTIFICATIONS: cache = &a->notifications; break;
        case VIEW_ANNOUNCEMENTS: cache = &a->announcements; break;
        case VIEW_SHOP: cache = &a->shop; break;
        default: return;
    }
    if (*cache == NULL) load_view(a, v);
}

static int current_list_count(app *a) {
    switch (a->view) {
        case VIEW_PROJECTS: return (int)json_array_count(a->projects);
        case VIEW_ORDERS: return (int)json_array_count(a->orders);
        case VIEW_NOTIFICATIONS: return (int)json_array_count(a->notifications);
        case VIEW_ANNOUNCEMENTS: return (int)json_array_count(a->announcements);
        case VIEW_SHOP: return a->shop ? (int)json_array_count(json_get(a->shop, "items")) : 0;
        default: return 0;
    }
}

static void cycle_region(app *a) {
    json_value *feed_regions = a->shop ? json_get(a->shop, "regions") : NULL;
    size_t count = json_array_count(feed_regions);

    if (count > 0) {
        int idx = 0;
        for (size_t i = 0; i < count; i++) {
            json_value *rv = json_array_at(feed_regions, i);
            if (rv->type == JSON_STRING && strcmp(a->cfg->region, rv->u.string) == 0) {
                idx = (int)i;
                break;
            }
        }
        idx = (idx + 1) % (int)count;
        json_value *next = json_array_at(feed_regions, (size_t)idx);
        if (next->type == JSON_STRING) {
            strncpy(a->cfg->region, next->u.string, sizeof(a->cfg->region) - 1);
            a->cfg->region[sizeof(a->cfg->region) - 1] = '\0';
        }
    } else {
        int idx = 0;
        for (int i = 0; i < FALLBACK_REGIONS_COUNT; i++)
            if (strcmp(a->cfg->region, FALLBACK_REGIONS[i]) == 0) { idx = i; break; }
        idx = (idx + 1) % FALLBACK_REGIONS_COUNT;
        strncpy(a->cfg->region, FALLBACK_REGIONS[idx], sizeof(a->cfg->region) - 1);
        a->cfg->region[sizeof(a->cfg->region) - 1] = '\0';
    }
    if(config_save(a->cfg)!=0){
        fprintf(stderr,"failed to save region configuration.\n");
    }
}

static void handle_key(app *a, int ch) {
    if (ch == 'q' || ch == 'Q') { a->running = 0; return; }

    if (ch == '?') {
        if (a->view == VIEW_HELP) a->view = a->prev_view;
        else { a->prev_view = a->view; a->view = VIEW_HELP; }
        return;
    }

    if (a->view == VIEW_HELP) {
        /* Any other key leaves help. */
        a->view = a->prev_view;
        return;
    }

    if (a->sidebar_focus) {
        switch (ch) {
            case KEY_UP: case 'k':
                a->menu_sel = (a->menu_sel - 1 + MENU_COUNT) % MENU_COUNT;
                break;
            case KEY_DOWN: case 'j':
                a->menu_sel = (a->menu_sel + 1) % MENU_COUNT;
                break;
            case '\n': case KEY_ENTER: case KEY_RIGHT: case 'l': {
                view_id v = MENU_VIEWS[a->menu_sel];
                a->view = v;
                a->sidebar_focus = 0;
                a->list_sel = 0;
                a->list_scroll = 0;
                ensure_loaded(a, v);
                break;
            }
            default: break;
        }
        return;
    }

    /* Content pane focused. */
    if (a->view == VIEW_PROJECT_DETAIL) {
        switch (ch) {
            case 27: case KEY_BACKSPACE: case 127: case 'h': case KEY_LEFT:
                a->view = VIEW_PROJECTS;
                break;
            case 'r':
                load_project_detail(a, a->project_detail_id);
                break;
            default: break;
        }
        return;
    }

    int count = current_list_count(a);
    switch (ch) {
        case KEY_UP: case 'k':
            if (a->list_sel > 0) a->list_sel--;
            break;
        case KEY_DOWN: case 'j':
            if (a->list_sel < count - 1) a->list_sel++;
            break;
        case 27: case KEY_BACKSPACE: case 127: case 'h': case KEY_LEFT:
            a->sidebar_focus = 1;
            break;
        case 'r':
            load_view(a, a->view);
            break;
        case 'R':
            if (a->view == VIEW_SHOP) cycle_region(a);
            break;
        case '\n': case KEY_ENTER: case 'l': case KEY_RIGHT:
            if (a->view == VIEW_PROJECTS && count > 0) {
                json_value *p = json_array_at(a->projects, (size_t)a->list_sel);
                long id = json_get_int(p, "id", -1);
                if (id >= 0) {
                    a->prev_view = VIEW_PROJECTS;
                    a->view = VIEW_PROJECT_DETAIL;
                    load_project_detail(a, id);
                }
            }
            break;
        default: break;
    }
}

void ui_run(crescent_config *cfg) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) init_colors();

    ensure_api_key(cfg);

    app a;
    memset(&a, 0, sizeof(a));
    a.cfg = cfg;
    a.view = VIEW_ACCOUNT;
    a.prev_view = VIEW_ACCOUNT;
    a.sidebar_focus = 1;
    a.menu_sel = 0;
    a.running = 1;
    set_status(&a, 0, "welcome. enter: open a section.");

    ensure_loaded(&a, VIEW_ACCOUNT);

    while (a.running) {
        render(&a);
        int ch = getch();
        handle_key(&a, ch);
    }

    free_view_caches(&a);
    endwin();
}