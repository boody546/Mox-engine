#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════════
#  Mox HUB — Modern Dashboard & Engine Launcher v3.5 (Pixel-Perfect Replica)
# ═══════════════════════════════════════════════════════════════════════════

import os, sys, json, time, io, queue, shutil, zipfile, tarfile, tempfile, threading, subprocess
import urllib.request, urllib.error
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from datetime import datetime

# ─── GitHub Release Direct ZIP URL ─────────────────────────────────
GITHUB_RELEASE_URL = "https://github.com/boody546/Mox-engine/releases/download/v1.0.0/MoxEngine.zip"

# ─── Paths Configuration ───────────────────────────────────────────
HUB_DIR       = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT  = os.path.dirname(HUB_DIR)
ENGINE_DIR    = os.path.join(PROJECT_ROOT, "Engine")
if not os.path.exists(ENGINE_DIR):
    ENGINE_DIR = PROJECT_ROOT

VERSIONS_DIR  = os.path.join(ENGINE_DIR, "versions")
PROJECTS_DIR  = os.path.join(PROJECT_ROOT, "Projects")
PROJECTS_DB   = os.path.join(HUB_DIR, "projects.json")
VERSIONS_DB   = os.path.join(HUB_DIR, "versions.json")

# ─── Dashboard Design Tokens (Modern Dark Palette) ────────────────
D = {
    "bg":           "#0f0f12",
    "sidebar":      "#141418",
    "sidebar_sel":  "#1f1f26",
    "topbar":       "#141418",
    "footer":       "#141418",
    "card":         "#18181c",
    "card_hover":   "#202026",
    "border":       "#27272a",
    "border_input": "#2a2a35",
    "border_radius": 8,
    "card_border_radius": 12,
    "input_border_radius": 6,
    "accent_blue":  "#3b82f6",
    "accent_hover": "#2563eb",
    "success":      "#10b981",
    "success_bg":   "#064e3b",
    "warning":      "#f59e0b",
    "danger":       "#ef4444",
    "fg":           "#f4f4f5",
    "fg2":          "#a1a1aa",
    "fg3":          "#71717a",
    "input_bg":     "#18181c",
    "badge_bg":     "#1e293b",
    "badge_fg":     "#60a5fa",
}

# ─── Project Templates ─────────────────────────────────────────────
TEMPLATES = [
    {
        "id": "demo",
        "name": "Mox Interactive Demo",
        "icon": "⭐",
        "desc": "Full Mox Engine demo with PlayerNode, Camera Shake, Particles, and NovaScript Box.",
        "tag":  "Demo",
    },
    {
        "id": "platformer",
        "name": "2D Platformer",
        "icon": "🎮",
        "desc": "Pre-configured tilemap, physics colliders, player controller, and camera follow.",
        "tag":  "Template",
    },
    {
        "id": "topdown",
        "name": "Top-Down RPG",
        "icon": "🗺️",
        "desc": "Top-down movement, dialogue system placeholder, and scene management.",
        "tag":  "Template",
    },
    {
        "id": "blank",
        "name": "Blank C++ Project",
        "icon": "🎨",
        "desc": "Minimal setup. Empty main_scene.json, scripts/ and assets/ folder ready.",
        "tag":  "Blank",
    },
]

_DEFAULT_RELEASES = [
    {
        "ver":    "Mox Engine 1",
        "label":  "Mox Engine 1 (Core Build)",
        "status": "installed",
        "date":   datetime.now().strftime("%Y-%m-%d"),
        "url":    None,
        "desc":   "Pre-compiled core Mox Engine binary with DirectX 11 & OpenGL hardware acceleration.",
        "path":   os.path.join(VERSIONS_DIR, "Mox Engine1", "MoxEngine.exe"),
    },
]


# ═══════════════════════════════════════════════════════════════════
# MAIN HUB DASHBOARD CLASS
# ═══════════════════════════════════════════════════════════════════
class MoxHubApp(tk.Tk):

    def __init__(self):
        super().__init__()
        self.title("Mox HUB — Management Console")
        self.geometry("1240x780")
        self.minsize(980, 640)
        self.configure(bg=D["bg"])

        self.active_nav     = tk.StringVar(value="projects")
        self.search_var     = tk.StringVar()
        self.projects       = []
        self.engine_versions= []
        self.active_version = "Mox Engine 1"
        self.sel_template   = tk.StringVar(value=TEMPLATES[0]["id"])

        self._ensure_dirs()
        self._auto_package_mox_engine()
        self._load_data()
        self._scan_installed_versions()
        self._build_dashboard_layout()
        self._switch_nav("projects")
        self._update_status()

    # ─── Setup & Storage ──────────────────────────────────────────
    def _ensure_dirs(self):
        os.makedirs(PROJECTS_DIR, exist_ok=True)
        os.makedirs(VERSIONS_DIR, exist_ok=True)

    def _auto_package_mox_engine(self):
        target_dir = os.path.join(VERSIONS_DIR, "Mox Engine1")
        os.makedirs(target_dir, exist_ok=True)
        target_exe = os.path.join(target_dir, "MoxEngine.exe")

        local_build = os.path.join(ENGINE_DIR, "build")
        sources = [
            os.path.join(local_build, "MoxEngine.exe"),
            os.path.join(local_build, "Nova2D.exe"),
            os.path.join(ENGINE_DIR, "MoxEngine.exe"),
            os.path.join(ENGINE_DIR, "Nova2D.exe"),
        ]
        found_src = next((s for s in sources if os.path.exists(s)), None)

        if os.path.exists(local_build):
            try:
                for item in os.listdir(local_build):
                    s = os.path.join(local_build, item)
                    d = os.path.join(target_dir, item)
                    if os.path.isdir(s):
                        shutil.copytree(s, d, dirs_exist_ok=True)
                    else:
                        shutil.copy2(s, d)
            except Exception: pass

        if found_src and not os.path.exists(target_exe):
            try: shutil.copy2(found_src, target_exe)
            except Exception: pass

    def _load_data(self):
        if os.path.exists(PROJECTS_DB):
            try:
                with open(PROJECTS_DB, "r", encoding="utf-8") as f:
                    self.projects = json.load(f)
            except Exception:
                self.projects = []
        else:
            self.projects = []

        self.engine_versions = list(_DEFAULT_RELEASES)
        self.active_version  = "Mox Engine 1"

        if os.path.exists(VERSIONS_DB):
            try:
                with open(VERSIONS_DB, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    saved = next((v for v in data.get("versions", []) if v.get("ver") in ("Mox Engine1", "Mox Engine 1")), None)
                    if saved: self.engine_versions = [saved]
            except Exception: pass

        self.active_version = "Mox Engine 1"

    def _save_data(self):
        with open(PROJECTS_DB, "w", encoding="utf-8") as f:
            json.dump(self.projects, f, ensure_ascii=False, indent=2)

        save_ver = []
        for v in self.engine_versions:
            save_ver.append({
                "ver":    v["ver"],
                "label":  v["label"],
                "status": v["status"],
                "date":   v["date"],
                "url":    v.get("url"),
                "desc":   v.get("desc", ""),
                "path":   v.get("path", ""),
            })

        with open(VERSIONS_DB, "w", encoding="utf-8") as f:
            json.dump({"active_version": self.active_version, "versions": save_ver},
                      f, ensure_ascii=False, indent=2)

    def _scan_installed_versions(self):
        mox1_path = os.path.join(VERSIONS_DIR, "Mox Engine1")
        mox1_exe  = self._find_exe_in_dir(mox1_path)

        self.engine_versions = [{
            "ver":    "Mox Engine 1",
            "label":  "Mox Engine 1 (Core Build)",
            "status": "installed" if mox1_exe else "not_installed",
            "date":   datetime.now().strftime("%Y-%m-%d"),
            "url":    None,
            "desc":   "Pre-compiled core Mox Engine binary — official release.",
            "path":   mox1_exe or mox1_path,
        }]
        self.active_version = "Mox Engine 1"

    def _find_exe_in_dir(self, directory):
        if not directory or not os.path.exists(directory): return None
        if os.path.isfile(directory) and directory.lower().endswith(".exe"): return directory
        mox_exe = os.path.join(directory, "MoxEngine.exe")
        if os.path.exists(mox_exe): return mox_exe
        nova_exe = os.path.join(directory, "Nova2D.exe")
        if os.path.exists(nova_exe): return nova_exe
        for sub in ("MoxEngine.exe", "Nova2D.exe"):
            b_exe = os.path.join(directory, "build", sub)
            if os.path.exists(b_exe): return b_exe
        for root, dirs, files in os.walk(directory):
            for file in files:
                if file.lower().endswith(".exe"): return os.path.join(root, file)
        return None

    def _get_active_exe(self):
        mox1_exe = self._find_exe_in_dir(os.path.join(VERSIONS_DIR, "Mox Engine1"))
        if mox1_exe: return mox1_exe
        for name in ("MoxEngine.exe", "Nova2D.exe"):
            p1 = os.path.join(ENGINE_DIR, "build", name)
            if os.path.exists(p1): return p1
            p2 = os.path.join(ENGINE_DIR, name)
            if os.path.exists(p2): return p2
        return None

    # ═══════════════════════════════════════════════════════════════
    # DASHBOARD LAYOUT BUILDER
    # ═══════════════════════════════════════════════════════════════
    def _build_dashboard_layout(self):
        # 1. Main horizontal container: [Sidebar | Main Area]
        main_box = tk.Frame(self, bg=D["bg"])
        main_box.pack(side="top", fill="both", expand=True)

        # ── Left Sidebar Navigation Panel ─────────────────────────
        self.sidebar = tk.Frame(main_box, bg=D["sidebar"], width=230)
        self.sidebar.pack(side="left", fill="y")
        self.sidebar.pack_propagate(False)
        self._build_sidebar()

        # ── Right Main Console Area ───────────────────────────────
        self.right_area = tk.Frame(main_box, bg=D["bg"])
        self.right_area.pack(side="left", fill="both", expand=True)

        # Top Bar Header
        self.topbar = tk.Frame(self.right_area, bg=D["topbar"], height=60, padx=20)
        self.topbar.pack(side="top", fill="x")
        self.topbar.pack_propagate(False)
        self._build_topbar()

        # Page Container
        self.page_container = tk.Frame(self.right_area, bg=D["bg"])
        self.page_container.pack(side="top", fill="both", expand=True)

        self.pages = {}
        for key in ("projects", "installs", "learn", "community", "settings"):
            page = tk.Frame(self.page_container, bg=D["bg"])
            self.pages[key] = page

        self._build_page_projects()
        self._build_page_installs()
        self._build_page_learn()
        self._build_page_community()
        self._build_page_settings()

        # 2. Bottom Fixed Engine Status Footer Bar
        self.footer = tk.Frame(self, bg=D["footer"], height=48, padx=20, pady=4)
        self.footer.pack(side="bottom", fill="x")
        self.footer.pack_propagate(False)
        self._build_footer()

    # ═══════════════════════════════════════════════════════════════
    # SIDEBAR NAVIGATION
    # ═══════════════════════════════════════════════════════════════
    def _build_sidebar(self):
        # Brand Header Logo
        brand = tk.Frame(self.sidebar, bg=D["sidebar"], padx=18, pady=18)
        brand.pack(fill="x")
        tk.Label(brand, text="⚡ Mox HUB", font=("Segoe UI", 13, "bold"),
                 bg=D["sidebar"], fg=D["fg"]).pack(anchor="w")
        tk.Label(brand, text="Management Console", font=("Segoe UI", 8),
                 bg=D["sidebar"], fg=D["fg3"]).pack(anchor="w", pady=(2, 0))

        tk.Frame(self.sidebar, bg=D["border"], height=1).pack(fill="x", padx=12, pady=(0, 10))

        # Main Navigation Group
        self.nav_btns = {}
        main_nav = [
            ("projects",  "📂", "Projects"),
            ("installs",  "⬇️", "Installs"),
            ("learn",     "🎓", "Learn"),
            ("community", "👥", "Community"),
            ("settings",  "⚙️", "Settings"),
        ]

        for key, icon, label in main_nav:
            btn = self._mk_nav_item(self.sidebar, key, icon, label)
            self.nav_btns[key] = btn

        # Spacer
        tk.Frame(self.sidebar, bg=D["sidebar"]).pack(fill="both", expand=True)

        tk.Frame(self.sidebar, bg=D["border"], height=1).pack(fill="x", padx=12, pady=6)

        # Bottom Navigation Group
        bottom_nav = [
            ("support",       "❓", "Support"),
            ("notifications", "🔔", "Notifications"),
        ]
        for key, icon, label in bottom_nav:
            self._mk_nav_item(self.sidebar, key, icon, label, is_bottom=True)

    def _mk_nav_item(self, parent, key, icon, label, is_bottom=False):
        card = tk.Frame(parent, bg=D["sidebar"], cursor="hand2", padx=6, pady=2)
        card.pack(fill="x", padx=8, pady=2)

        inner = tk.Frame(card, bg=D["sidebar"], padx=12, pady=8)
        inner.pack(fill="x")

        icon_lbl = tk.Label(inner, text=icon, font=("Segoe UI", 11),
                            bg=D["sidebar"], fg=D["fg2"])
        icon_lbl.pack(side="left")

        text_lbl = tk.Label(inner, text=f"  {label}", font=("Segoe UI", 9, "bold" if not is_bottom else "normal"),
                            bg=D["sidebar"], fg=D["fg2"])
        text_lbl.pack(side="left")

        cmd = (lambda e, k=key: self._on_bottom_nav(k)) if is_bottom else (lambda e, k=key: self._switch_nav(k))

        for w in (card, inner, icon_lbl, text_lbl):
            w.bind("<Button-1>", cmd)
            w.bind("<Enter>", lambda e, c=card, i=inner, il=icon_lbl, tl=text_lbl:
                   self._nav_item_hover(c, i, il, tl, True, key, is_bottom))
            w.bind("<Leave>", lambda e, c=card, i=inner, il=icon_lbl, tl=text_lbl:
                   self._nav_item_hover(c, i, il, tl, False, key, is_bottom))

        return (card, inner, icon_lbl, text_lbl)

    def _nav_item_hover(self, card, inner, icon_lbl, text_lbl, enter, key, is_bottom):
        if not is_bottom and self.active_nav.get() == key:
            return
        bg = D["sidebar_sel"] if enter else D["sidebar"]
        fg = D["fg"] if enter else D["fg2"]
        card.configure(bg=bg)
        inner.configure(bg=bg)
        icon_lbl.configure(bg=bg, fg=fg)
        text_lbl.configure(bg=bg, fg=fg)

    def _switch_nav(self, key):
        prev = self.active_nav.get()
        self.active_nav.set(key)

        if prev in self.nav_btns:
            card, inner, icon_lbl, text_lbl = self.nav_btns[prev]
            card.configure(bg=D["sidebar"])
            inner.configure(bg=D["sidebar"])
            icon_lbl.configure(bg=D["sidebar"], fg=D["fg2"])
            text_lbl.configure(bg=D["sidebar"], fg=D["fg2"])

        if key in self.nav_btns:
            card, inner, icon_lbl, text_lbl = self.nav_btns[key]
            card.configure(bg=D["sidebar_sel"])
            inner.configure(bg=D["sidebar_sel"])
            icon_lbl.configure(bg=D["sidebar_sel"], fg=D["accent_blue"])
            text_lbl.configure(bg=D["sidebar_sel"], fg=D["accent_blue"])

        for k, p in self.pages.items():
            p.pack_forget()
        if key in self.pages:
            self.pages[key].pack(fill="both", expand=True)

    def _on_bottom_nav(self, key):
        if key == "support":
            messagebox.showinfo("Mox Support", "Mox Engine Documentation & Support\n\nWebsite: github.com/boody546/Mox-engine\nDocs: Readme & SKILL.md guide")
        elif key == "notifications":
            messagebox.showinfo("Notifications", "🔔 No new system notifications.\n\nAll engine binaries and dependencies are up to date.")

    # ═══════════════════════════════════════════════════════════════
    # TOP BAR HEADER
    # ═══════════════════════════════════════════════════════════════
    def _build_topbar(self):
        # Left/Center: Search Bar
        search_box = tk.Frame(self.topbar, bg=D["card"], bd=1, relief="solid",
                              highlightbackground=D["border_input"], highlightthickness=1)
        search_box.pack(side="left", pady=10)

        tk.Label(search_box, text=" 🔍 ", bg=D["card"], fg=D["fg3"],
                 font=("Segoe UI", 10)).pack(side="left")

        e_search = tk.Entry(search_box, textvariable=self.search_var, font=("Segoe UI", 9),
                            bg=D["card"], fg=D["fg"], insertbackground=D["fg"],
                            bd=0, relief="flat", width=34)
        e_search.pack(side="left", padx=(0, 10), pady=6)
        self.search_var.trace_add("write", lambda *_: self._render_project_list())

        # Placeholder label effect
        def _on_focus_in(e):
            pass
        e_search.bind("<FocusIn>", _on_focus_in)

        # Right Action Cluster: [+ Open Project] [+ New Project] [User profile]
        right_cluster = tk.Frame(self.topbar, bg=D["topbar"])
        right_cluster.pack(side="right", pady=10)

        self._mk_btn(right_cluster, "+ Open Project", self._open_existing_project,
                     bg=D["card"], fg=D["fg"], padx=12, pady=6,
                     font=("Segoe UI", 9, "bold")).pack(side="left", padx=4)

        self._mk_btn(right_cluster, "+ New Project", self._open_new_project_dialog,
                     bg=D["accent_blue"], fg="#ffffff", padx=14, pady=6,
                     font=("Segoe UI", 9, "bold")).pack(side="left", padx=4)

        # Profile / Notifications Icons
        tk.Label(right_cluster, text="🔔", font=("Segoe UI", 11),
                 bg=D["topbar"], fg=D["fg2"], cursor="hand2").pack(side="left", padx=(12, 6))
        tk.Label(right_cluster, text="👤", font=("Segoe UI", 12),
                 bg=D["topbar"], fg=D["fg"], cursor="hand2").pack(side="left", padx=4)

    # ═══════════════════════════════════════════════════════════════
    # BOTTOM ENGINE STATUS FOOTER BAR
    # ═══════════════════════════════════════════════════════════════
    def _build_footer(self):
        # Left: Active Engine label
        left_lbl = tk.Label(self.footer, text="Current Installed Engine: Mox Engine 1",
                            font=("Segoe UI", 9), bg=D["footer"], fg=D["fg2"])
        left_lbl.pack(side="left")

        # Center: Bright Green Status Badge
        self.lbl_status = tk.Label(self.footer, text="🟢 MOX ENGINE READY (Mox Engine 1)",
                                   font=("Segoe UI", 9, "bold"),
                                   bg=D["footer"], fg=D["success"])
        self.lbl_status.pack(side="left", expand=True)

        # Right: Highlighted Launch Button
        self.btn_launch = self._mk_btn(self.footer, "🚀 Launch Mox Engine",
                                       self.launch_engine,
                                       bg=D["success"], fg="#062e1e",
                                       font=("Segoe UI", 9, "bold"), padx=16, pady=5)
        self.btn_launch.pack(side="right")

    # ═══════════════════════════════════════════════════════════════
    # PAGE: PROJECTS
    # ═══════════════════════════════════════════════════════════════
    def _build_page_projects(self):
        page = self.pages["projects"]

        # Title Header
        hdr = tk.Frame(page, bg=D["bg"], padx=24, pady=16)
        hdr.pack(fill="x")
        tk.Label(hdr, text="Projects", font=("Segoe UI", 18, "bold"),
                 bg=D["bg"], fg=D["fg"]).pack(side="left")

        # Project Cards List Container
        self.projects_scroll = tk.Canvas(page, bg=D["bg"], highlightthickness=0, bd=0)
        sb = ttk.Scrollbar(page, style="Unity.Vertical.TScrollbar", orient="vertical",
                           command=self.projects_scroll.yview)
        self.projects_inner = tk.Frame(self.projects_scroll, bg=D["bg"], padx=24)

        self.projects_inner.bind("<Configure>", lambda e: self.projects_scroll.configure(
            scrollregion=self.projects_scroll.bbox("all")))
        self.projects_scroll.create_window((0, 0), window=self.projects_inner, anchor="nw")
        self.projects_scroll.configure(yscrollcommand=sb.set)

        self.projects_scroll.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")

        self._render_project_list()

    def _render_project_list(self):
        for w in self.projects_inner.winfo_children():
            w.destroy()

        search = self.search_var.get().lower()
        filtered = [p for p in self.projects if not search or search in p["name"].lower() or search in p.get("path","").lower()]

        if not filtered:
            empty = tk.Frame(self.projects_inner, bg=D["bg"], pady=60)
            empty.pack(expand=True, fill="both")
            tk.Label(empty, text="🎮", font=("Segoe UI", 42),
                     bg=D["bg"], fg=D["fg3"]).pack()
            tk.Label(empty, text="No Projects Found",
                     font=("Segoe UI", 15, "bold"), bg=D["bg"], fg=D["fg2"]).pack(pady=4)
            tk.Label(empty, text="Click '+ New Project' in top-right to create a project.",
                     font=("Segoe UI", 9), bg=D["bg"], fg=D["fg3"]).pack()
            return

        # Table Header Row
        hdr = tk.Frame(self.projects_inner, bg=D["bg"], pady=6)
        hdr.pack(fill="x")

        tk.Label(hdr, text="Project Name", font=("Segoe UI", 8, "bold"),
                 bg=D["bg"], fg=D["fg3"], width=32, anchor="w").pack(side="left")
        tk.Label(hdr, text="Target Engine", font=("Segoe UI", 8, "bold"),
                 bg=D["bg"], fg=D["fg3"], width=16, anchor="center").pack(side="left")
        tk.Label(hdr, text="Last Opened", font=("Segoe UI", 8, "bold"),
                 bg=D["bg"], fg=D["fg3"], width=18, anchor="center").pack(side="left")
        tk.Label(hdr, text="Actions", font=("Segoe UI", 8, "bold"),
                 bg=D["bg"], fg=D["fg3"], anchor="e").pack(side="right", padx=20)

        tk.Frame(self.projects_inner, bg=D["border"], height=1).pack(fill="x", pady=(0, 8))

        for proj in reversed(filtered):
            self._render_project_card(proj)

    def _render_project_card(self, proj):
        # Card container #18181c with border #27272a
        card = tk.Frame(self.projects_inner, bg=D["card"], bd=1, relief="solid",
                        highlightbackground=D["border_input"], highlightthickness=1,
                        padx=12, pady=10, cursor="hand2")
        card.pack(fill="x", pady=4)

        # Left Column: Project Icon Tile + Name & Path
        col_left = tk.Frame(card, bg=D["card"], width=300)
        col_left.pack(side="left", anchor="w")

        # Icon tile (square tile)
        tile = tk.Frame(col_left, bg="#222228", width=40, height=40, bd=0)
        tile.pack(side="left", padx=(0, 10))
        tile.pack_propagate(False)
        tk.Label(tile, text=proj.get("icon", "🎮"), font=("Segoe UI", 16),
                 bg="#222228", fg=D["fg"]).pack(expand=True)

        name_box = tk.Frame(col_left, bg=D["card"])
        name_box.pack(side="left", anchor="w")
        tk.Label(name_box, text=proj["name"], font=("Segoe UI", 10, "bold"),
                 bg=D["card"], fg=D["fg"]).pack(anchor="w")

        path_text = proj.get("path", "")
        if len(path_text) > 42:
            path_text = path_text[:20] + "..." + path_text[-20:]
        tk.Label(name_box, text=path_text, font=("Consolas", 8),
                 bg=D["card"], fg=D["fg3"]).pack(anchor="w")

        # Column 2: Target Engine Badge
        col_eng = tk.Frame(card, bg=D["card"], width=160)
        col_eng.pack(side="left", expand=True)
        ver_tag = proj.get("engine_ver", self.active_version)
        badge = tk.Label(col_eng, text=ver_tag, font=("Segoe UI", 8, "bold"),
                         bg=D["badge_bg"], fg=D["badge_fg"], padx=10, pady=2)
        badge.pack()

        # Column 3: Last Opened
        col_opened = tk.Frame(card, bg=D["card"], width=160)
        col_opened.pack(side="left", expand=True)
        tk.Label(col_opened, text=proj.get("last_opened", "Never"),
                 font=("Segoe UI", 8), bg=D["card"], fg=D["fg2"]).pack()

        # Column 4: Actions (📝 Editor | ▶ Run | ...)
        col_act = tk.Frame(card, bg=D["card"])
        col_act.pack(side="right")

        self._mk_btn(col_act, "📝 Editor", lambda p=proj: self._open_editor(p),
                     bg=D["card_hover"], fg=D["fg"], padx=10, pady=4,
                     font=("Segoe UI", 8, "bold")).pack(side="left", padx=3)

        self._mk_btn(col_act, "▶ Run", lambda p=proj: self._launch_project(p),
                     bg=D["success"], fg="#062e1e", padx=12, pady=4,
                     font=("Segoe UI", 8, "bold")).pack(side="left", padx=3)

        opts_btn = mk_btn(col_act, "⚙️", lambda p=proj, b=card: self._show_project_ctx_menu(p, b),
                          bg=D["card"], fg=D["fg2"], padx=6, pady=4, font=("Segoe UI", 9))
        opts_btn.pack(side="left")

        # Card hover effect
        def _on_enter(e):
            card.configure(bg=D["card_hover"])
            for w in (col_left, name_box, col_eng, col_opened, col_act):
                try: w.configure(bg=D["card_hover"])
                except Exception: pass

        def _on_leave(e):
            card.configure(bg=D["card"])
            for w in (col_left, name_box, col_eng, col_opened, col_act):
                try: w.configure(bg=D["card"])
                except Exception: pass

        card.bind("<Enter>", _on_enter)
        card.bind("<Leave>", _on_leave)

    def _show_project_ctx_menu(self, proj, widget):
        m = tk.Menu(self, tearoff=0, bg=D["card"], fg=D["fg"],
                    activebackground=D["accent_blue"], font=("Segoe UI", 9))
        m.add_command(label="📝 Open 2D Editor",   command=lambda: self._open_editor(proj))
        m.add_command(label="▶ Run Game Binary",    command=lambda: self._launch_project(proj))
        m.add_separator()
        m.add_command(label="📂 Open in Explorer", command=lambda: os.startfile(proj["path"]))
        m.add_command(label="💻 Open in VS Code",   command=lambda: self._open_vscode(proj))
        m.add_separator()
        m.add_command(label="🗑️ Remove from List", command=lambda: self._remove_project(proj))
        m.post(self.winfo_pointerx(), self.winfo_pointery())

    # ═══════════════════════════════════════════════════════════════
    # NEW PROJECT DIALOG
    # ═══════════════════════════════════════════════════════════════
    def _open_new_project_dialog(self):
        win = tk.Toplevel(self)
        win.title("Create New Project")
        win.geometry("580x460")
        win.configure(bg=D["bg"])
        win.resizable(False, False)
        win.grab_set()

        tk.Label(win, text="Create New Project", font=("Segoe UI", 14, "bold"),
                 bg=D["bg"], fg=D["fg"]).pack(pady=(18, 2), padx=20, anchor="w")
        tk.Label(win, text="Select template and project folder.",
                 font=("Segoe UI", 9), bg=D["bg"], fg=D["fg2"]).pack(padx=20, anchor="w", pady=(0, 12))

        tpl_frame = tk.Frame(win, bg=D["bg"], padx=20)
        tpl_frame.pack(fill="x")
        for tpl in TEMPLATES:
            card = tk.Frame(tpl_frame, bg=D["card"], padx=10, pady=6, bd=1, relief="solid",
                            highlightbackground=D["border"], highlightthickness=1)
            card.pack(fill="x", pady=2)
            tk.Radiobutton(card, variable=self.sel_template, value=tpl["id"],
                           bg=D["card"], activebackground=D["card"], selectcolor=D["accent_blue"]).pack(side="left")
            tk.Label(card, text=f"{tpl['icon']}  {tpl['name']}", font=("Segoe UI", 9, "bold"),
                     bg=D["card"], fg=D["fg"]).pack(side="left")
            tk.Label(card, text=tpl["desc"], font=("Segoe UI", 8),
                     bg=D["card"], fg=D["fg2"]).pack(side="left", padx=8)

        form = tk.Frame(win, bg=D["bg"], padx=20, pady=10)
        form.pack(fill="x")

        tk.Label(form, text="Project Name", font=("Segoe UI", 9, "bold"),
                 bg=D["bg"], fg=D["fg2"]).pack(anchor="w")
        name_var = tk.StringVar(value="MyNovaGame")
        e_name = tk.Entry(form, textvariable=name_var, font=("Segoe UI", 10),
                          bg=D["input_bg"], fg=D["fg"], insertbackground=D["fg"],
                          bd=1, relief="solid", highlightthickness=0)
        e_name.pack(fill="x", pady=(3, 8))

        tk.Label(form, text="Location", font=("Segoe UI", 9, "bold"),
                 bg=D["bg"], fg=D["fg2"]).pack(anchor="w")
        loc_var = tk.StringVar(value=PROJECTS_DIR)
        loc_row = tk.Frame(form, bg=D["bg"])
        loc_row.pack(fill="x", pady=(3, 0))
        tk.Entry(loc_row, textvariable=loc_var, font=("Segoe UI", 9),
                 bg=D["input_bg"], fg=D["fg"], insertbackground=D["fg"],
                 bd=1, relief="solid", highlightthickness=0).pack(side="left", fill="x", expand=True)
        self._mk_btn(loc_row, "Browse...",
                     lambda: loc_var.set(filedialog.askdirectory(initialdir=PROJECTS_DIR) or loc_var.get()),
                     bg=D["card"], fg=D["fg"], padx=8, pady=3).pack(side="left", padx=(4, 0))

        bot = tk.Frame(win, bg=D["bg"], padx=20, pady=12)
        bot.pack(fill="x", side="bottom")
        self._mk_btn(bot, "Cancel", win.destroy, bg=D["card"], fg=D["fg"], padx=12, pady=5).pack(side="right", padx=4)
        self._mk_btn(bot, "+ Create Project", lambda: self._create_project(name_var.get(), loc_var.get(), win),
                     bg=D["accent_blue"], fg="#fff", padx=14, pady=5, font=("Segoe UI", 9, "bold")).pack(side="right")

    def _create_project(self, name, location, win):
        name = name.strip()
        if not name:
            messagebox.showwarning("Invalid Name", "Project name cannot be empty.", parent=win)
            return
        proj_path = os.path.join(location, name)
        if os.path.exists(proj_path):
            messagebox.showwarning("Exists", f"Directory already exists:\n{proj_path}", parent=win)
            return

        try:
            os.makedirs(proj_path)
            os.makedirs(os.path.join(proj_path, "scripts"))
            os.makedirs(os.path.join(proj_path, "assets"))

            tpl_id = self.sel_template.get()
            tpl    = next((t for t in TEMPLATES if t["id"] == tpl_id), TEMPLATES[-1])
            with open(os.path.join(proj_path, "scripts", "main.nova"), "w", encoding="utf-8") as f:
                f.write(f"# {name} — {tpl['name']}\n\nfunc _ready():\n    print(\"Hello from {name}!\")\n\nfunc _process(delta):\n    pass\n")

            entry = {
                "name":        name,
                "path":        proj_path,
                "template":    tpl["name"],
                "icon":        tpl["icon"],
                "engine_ver":  self.active_version,
                "created":     datetime.now().strftime("%Y-%m-%d"),
                "last_opened": datetime.now().strftime("%Y-%m-%d %H:%M"),
            }
            self.projects.append(entry)
            self._save_data()
            win.destroy()
            self._render_project_list()
        except Exception as e:
            messagebox.showerror("Error", str(e), parent=win)

    def _open_existing_project(self):
        path = filedialog.askdirectory(initialdir=PROJECT_ROOT, title="Select Mox Engine Project Folder")
        if not path: return
        name = os.path.basename(path)
        if any(p["path"] == path for p in self.projects):
            messagebox.showinfo("Already Added", f"'{name}' is already in your project list.")
            return
        entry = {
            "name":        name,
            "path":        path,
            "template":    "Existing",
            "icon":        "📁",
            "engine_ver":  self.active_version,
            "created":     "Unknown",
            "last_opened": datetime.now().strftime("%Y-%m-%d %H:%M"),
        }
        self.projects.append(entry)
        self._save_data()
        self._render_project_list()

    # ═══════════════════════════════════════════════════════════════
    # EXECUTION FLOW VERIFICATION (Editor & Run Game)
    # ═══════════════════════════════════════════════════════════════
    def _open_editor(self, proj):
        """Flow 1: Launches editor.py with --project argument."""
        proj_path = proj.get("path", "")
        editor_script = os.path.join(HUB_DIR, "editor.py")
        if not os.path.exists(editor_script):
            messagebox.showerror("Editor Error", f"Editor script not found:\n{editor_script}")
            return
        try:
            subprocess.Popen([sys.executable, editor_script, "--project", proj_path])
            proj["last_opened"] = datetime.now().strftime("%Y-%m-%d %H:%M")
            self._save_data()
            self._render_project_list()
        except Exception as e:
            messagebox.showerror("Error", f"Failed to launch 2D Editor:\n{e}")

    def _launch_project(self, proj):
        """Flow 2: Executes MoxEngine.exe with --project argument."""
        exe_path = self._get_active_exe()
        if not exe_path or not os.path.exists(exe_path):
            messagebox.showwarning("Engine Required",
                f"No binary executable found for {self.active_version}.\n\n"
                "Please go to 'Installs' tab to package or download MoxEngine.exe.")
            return

        try:
            work_dir = os.path.dirname(exe_path)
            norm_proj_path = os.path.normpath(proj.get("path", ""))
            subprocess.Popen([exe_path, "--project", norm_proj_path], cwd=work_dir)
            proj["last_opened"] = datetime.now().strftime("%Y-%m-%d %H:%M")
            self._save_data()
            self._render_project_list()
        except Exception as e:
            messagebox.showerror("Launch Error", f"Could not launch executable:\n{e}")

    def _open_vscode(self, proj):
        path = proj.get("path", "")
        try: subprocess.Popen(["code", path], shell=True)
        except Exception as e: messagebox.showerror("Error", str(e))

    def _remove_project(self, proj):
        if messagebox.askyesno("Remove", f"Remove '{proj['name']}' from project list?"):
            self.projects = [p for p in self.projects if p["path"] != proj["path"]]
            self._save_data()
            self._render_project_list()

    # ═══════════════════════════════════════════════════════════════
    # PAGE: INSTALLS
    # ═══════════════════════════════════════════════════════════════
    def _build_page_installs(self):
        page = self.pages["installs"]

        hdr = tk.Frame(page, bg=D["bg"], padx=24, pady=16)
        hdr.pack(fill="x")
        tk.Label(hdr, text="Mox Engine Installs", font=("Segoe UI", 18, "bold"),
                 bg=D["bg"], fg=D["fg"]).pack(side="left")

        btn_row = tk.Frame(hdr, bg=D["bg"])
        btn_row.pack(side="right")
        self._mk_btn(btn_row, "📦 Package Local Build", self._bundle_local_build_dialog,
                     bg=D["accent_blue"], fg="#fff", padx=12, pady=5,
                     font=("Segoe UI", 9, "bold")).pack(side="left", padx=3)
        self._mk_btn(btn_row, "📂 Install from ZIP", self._install_local_dialog,
                     bg=D["card"], fg=D["fg"], padx=10, pady=5).pack(side="left")

        self.ver_list_frame = tk.Frame(page, bg=D["bg"], padx=24)
        self.ver_list_frame.pack(fill="both", expand=True)
        self._render_version_list()

    def _render_version_list(self):
        for w in self.ver_list_frame.winfo_children():
            w.destroy()

        for ver_info in self.engine_versions:
            is_installed = ver_info.get("status") == "installed"
            card = tk.Frame(self.ver_list_frame, bg=D["card"], bd=1, relief="solid",
                            highlightbackground=D["border"], highlightthickness=1, padx=16, pady=12)
            card.pack(fill="x", pady=4)

            left = tk.Frame(card, bg=D["card"])
            left.pack(side="left", fill="x", expand=True)
            tk.Label(left, text=ver_info["label"], font=("Segoe UI", 11, "bold"),
                     bg=D["card"], fg=D["fg"]).pack(anchor="w")
            tk.Label(left, text=ver_info.get("desc",""), font=("Segoe UI", 8),
                     bg=D["card"], fg=D["fg2"]).pack(anchor="w")

            status_txt, status_col = ("🟢 Installed", D["success"]) if is_installed else ("🟡 Not Installed", D["warning"])
            tk.Label(card, text=status_txt, font=("Segoe UI", 9, "bold"),
                     bg=D["card"], fg=status_col).pack(side="left", padx=20)

            btn_row = tk.Frame(card, bg=D["card"])
            btn_row.pack(side="right")

            if is_installed:
                self._mk_btn(btn_row, "▶ Test Run", lambda v=ver_info: self._test_run_version(v),
                             bg=D["card_hover"], fg=D["fg"], padx=10, pady=4).pack(side="left", padx=3)
                self._mk_btn(btn_row, "📦 Re-Package", lambda: self._bundle_local_build_dialog(),
                             bg=D["card"], fg=D["badge_fg"], padx=10, pady=4).pack(side="left", padx=3)
                self._mk_btn(btn_row, "🗑️ Uninstall", lambda: self._uninstall_engine(),
                             bg=D["card"], fg=D["danger"], padx=10, pady=4).pack(side="left")
            else:
                self._mk_btn(btn_row, "⬇ Download Release", lambda: self._install_remote_engine(),
                             bg=D["success"], fg="#062e1e", padx=14, pady=5, font=("Segoe UI", 9, "bold")).pack(side="left", padx=3)

    def _test_run_version(self, ver_info):
        path = ver_info.get("path")
        exe  = self._find_exe_in_dir(path) if (path and os.path.isdir(path)) else path
        if not exe or not os.path.exists(exe):
            exe = os.path.join(VERSIONS_DIR, "Mox Engine1", "MoxEngine.exe")
        if not exe or not os.path.exists(exe):
            messagebox.showwarning("Not Found", f"No MoxEngine.exe found in:\n{path}")
            return
        try: subprocess.Popen([exe], cwd=os.path.dirname(exe))
        except Exception as e: messagebox.showerror("Error", str(e))

    def _uninstall_engine(self):
        if not messagebox.askyesno("Uninstall Engine", "Delete installed Mox Engine binaries?"): return
        try:
            subprocess.run(["taskkill", "/F", "/IM", "MoxEngine.exe"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception: pass
        target_dir = os.path.join(VERSIONS_DIR, "Mox Engine1")
        if os.path.exists(target_dir): shutil.rmtree(target_dir, ignore_errors=True)
        self._scan_installed_versions()
        self._save_data()
        self._render_version_list()
        self._update_status()

    def _bundle_local_build_dialog(self):
        ver = "Mox Engine 1"
        target_dir = os.path.join(VERSIONS_DIR, "Mox Engine1")
        os.makedirs(target_dir, exist_ok=True)
        target_exe = os.path.join(target_dir, "MoxEngine.exe")
        local_build = os.path.join(ENGINE_DIR, "build")

        if os.path.exists(local_build):
            for item in os.listdir(local_build):
                s = os.path.join(local_build, item)
                d = os.path.join(target_dir, item)
                if os.path.isdir(s): shutil.copytree(s, d, dirs_exist_ok=True)
                else: shutil.copy2(s, d)

        self._scan_installed_versions()
        self._save_data()
        self._render_version_list()
        self._update_status()
        messagebox.showinfo("Packaged", "Local build packaged into Mox Engine 1 directory!")

    def _install_local_dialog(self):
        f = filedialog.askopenfilename(filetypes=[("Archives", "*.zip"), ("All Files", "*.*")])
        if not f: return
        target_dir = os.path.join(VERSIONS_DIR, "Mox Engine1")
        os.makedirs(target_dir, exist_ok=True)
        try:
            with zipfile.ZipFile(f, "r") as zf: zf.extractall(target_dir)
            self._scan_installed_versions()
            self._save_data()
            self._render_version_list()
            self._update_status()
            messagebox.showinfo("Success", "Engine installed from local ZIP!")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def _install_remote_engine(self):
        target_dir = os.path.join(VERSIONS_DIR, "Mox Engine1")
        os.makedirs(target_dir, exist_ok=True)
        win = tk.Toplevel(self); win.title("Downloading Mox Engine..."); win.geometry("460x180"); win.configure(bg=D["bg"]); win.grab_set()
        tk.Label(win, text="⬇ Downloading Mox Engine...", font=("Segoe UI", 12, "bold"), bg=D["bg"], fg=D["fg"]).pack(pady=16)
        pbar = ttk.Progressbar(win, mode="indeterminate", length=360); pbar.pack(pady=10); pbar.start(10)

        def _run():
            tmp_zip = os.path.join(tempfile.gettempdir(), "mox_engine.zip")
            try:
                urllib.request.urlretrieve(GITHUB_RELEASE_URL, tmp_zip)
                with zipfile.ZipFile(tmp_zip, "r") as zf: zf.extractall(target_dir)
                os.remove(tmp_zip)
                self.after(0, lambda: [win.destroy(), self._scan_installed_versions(), self._save_data(), self._render_version_list(), self._update_status(), messagebox.showinfo("Installed", "Mox Engine 1 Installed!")])
            except Exception as e:
                self.after(0, lambda err=e: [win.destroy(), messagebox.showerror("Download Error", str(err))])

        threading.Thread(target=_run, daemon=True).start()

    # ═══════════════════════════════════════════════════════════════
    # PAGE: LEARN & COMMUNITY & SETTINGS
    # ═══════════════════════════════════════════════════════════════
    def _build_page_learn(self):
        page = self.pages["learn"]
        hdr = tk.Frame(page, bg=D["bg"], padx=24, pady=16); hdr.pack(fill="x")
        tk.Label(hdr, text="🎓 Learn Mox Engine", font=("Segoe UI", 18, "bold"), bg=D["bg"], fg=D["fg"]).pack(side="left")

        body = tk.Frame(page, bg=D["bg"], padx=24); body.pack(fill="both", expand=True)
        tutorials = [
            ("⭐ Quick Start Guide", "Learn how to build scenes, add Node2D elements, and write NovaScript logic."),
            ("🎮 2D Physics & Collisions", "Configuring RigidBody2D, StaticBody2D, Area2D and custom bounciness."),
            ("🎨 TileMap & World Building", "Using the built-in TileMap Painter Drawer and tile palettes."),
            ("🎬 Animation & Sound Effects", "Keyframing Sprite2D properties and adding AudioStreamPlayer2D sounds."),
        ]
        for title, desc in tutorials:
            card = tk.Frame(body, bg=D["card"], bd=1, relief="solid", highlightbackground=D["border"], highlightthickness=1, padx=16, pady=12)
            card.pack(fill="x", pady=4)
            tk.Label(card, text=title, font=("Segoe UI", 11, "bold"), bg=D["card"], fg=D["accent_blue"]).pack(anchor="w")
            tk.Label(card, text=desc, font=("Segoe UI", 9), bg=D["card"], fg=D["fg2"]).pack(anchor="w", pady=(2, 0))

    def _build_page_community(self):
        page = self.pages["community"]
        hdr = tk.Frame(page, bg=D["bg"], padx=24, pady=16); hdr.pack(fill="x")
        tk.Label(hdr, text="👥 Community & Links", font=("Segoe UI", 18, "bold"), bg=D["bg"], fg=D["fg"]).pack(side="left")

        body = tk.Frame(page, bg=D["bg"], padx=24); body.pack(fill="both", expand=True)
        links = [
            ("🌐 Official GitHub Repository", "github.com/boody546/Mox-engine"),
            ("📦 Latest Engine Release", "github.com/boody546/Mox-engine/releases/tag/v1.0.0"),
            ("📜 Documentation & Guides", "github.com/boody546/Mox-engine#readme"),
        ]
        for title, url in links:
            card = tk.Frame(body, bg=D["card"], bd=1, relief="solid", highlightbackground=D["border"], highlightthickness=1, padx=16, pady=12)
            card.pack(fill="x", pady=4)
            tk.Label(card, text=title, font=("Segoe UI", 11, "bold"), bg=D["card"], fg=D["fg"]).pack(anchor="w")
            tk.Label(card, text=url, font=("Consolas", 9), bg=D["card"], fg=D["accent_blue"]).pack(anchor="w", pady=(2, 0))

    def _build_page_settings(self):
        page = self.pages["settings"]
        hdr = tk.Frame(page, bg=D["bg"], padx=24, pady=16); hdr.pack(fill="x")
        tk.Label(hdr, text="Settings", font=("Segoe UI", 18, "bold"), bg=D["bg"], fg=D["fg"]).pack(side="left")

        container = tk.Frame(page, bg=D["bg"], padx=24); container.pack(fill="both", expand=True)
        rows = [
            ("Projects Storage Directory", PROJECTS_DIR),
            ("Engine Versions Storage", VERSIONS_DIR),
            ("Projects Database File", PROJECTS_DB),
            ("Active Engine Release", "Mox Engine 1"),
        ]
        card = tk.Frame(container, bg=D["card"], bd=1, relief="solid", highlightbackground=D["border"], highlightthickness=1, padx=16, pady=12)
        card.pack(fill="x", pady=6)
        for k, v in rows:
            r = tk.Frame(card, bg=D["card"]); r.pack(fill="x", pady=4)
            tk.Label(r, text=k, font=("Segoe UI", 9), bg=D["card"], fg=D["fg2"], width=24, anchor="w").pack(side="left")
            tk.Label(r, text=v, font=("Consolas", 8), bg=D["card"], fg=D["fg"]).pack(side="left")

    # ═══════════════════════════════════════════════════════════════
    # STATUS & UTILITIES
    # ═══════════════════════════════════════════════════════════════
    def launch_engine(self):
        exe_path = self._get_active_exe()
        if not exe_path or not os.path.exists(exe_path):
            messagebox.showwarning("Engine Required", "No binary executable found for Mox Engine 1.")
            return
        try:
            subprocess.Popen([exe_path], cwd=os.path.dirname(exe_path))
        except Exception as e: messagebox.showerror("Error", str(e))

    def _update_status(self):
        exe_path = self._get_active_exe()
        if exe_path and os.path.exists(exe_path):
            self.lbl_status.configure(text="🟢 MOX ENGINE READY (Mox Engine 1)", fg=D["success"])
        else:
            self.lbl_status.configure(text="▲ NO MOX ENGINE BINARY FOUND", fg=D["warning"])

    def _mk_btn(self, parent, text, command, bg=None, fg=None, font=("Segoe UI", 9), padx=10, pady=4):
        bg = bg or D["card"]
        fg = fg or D["fg"]
        btn = tk.Button(parent, text=text, command=command, bg=bg, fg=fg,
                        activebackground=self._lighten(bg), activeforeground=fg,
                        font=font, relief="flat", padx=padx, pady=pady, cursor="hand2", bd=0,
                        highlightthickness=0)
        btn.bind("<Enter>", lambda e: btn.configure(bg=self._lighten(btn.cget("bg"))))
        btn.bind("<Leave>", lambda e: btn.configure(bg=bg))
        return btn

    @staticmethod
    def _lighten(hex_color):
        try:
            hex_color = hex_color.lstrip("#")
            r, g, b = (int(hex_color[i:i+2], 16) for i in (0, 2, 4))
            r, g, b = min(255, r+20), min(255, g+20), min(255, b+20)
            return f"#{r:02x}{g:02x}{b:02x}"
        except Exception:
            return hex_color


def mk_btn(parent, text, command, bg=None, fg=None, font=("Segoe UI", 9), padx=10, pady=4):
    bg = bg or D["card"]
    fg = fg or D["fg"]
    return tk.Button(parent, text=text, command=command, bg=bg, fg=fg,
                     activebackground=D["card_hover"], activeforeground=fg,
                     font=font, relief="flat", padx=padx, pady=pady, cursor="hand2", bd=0)


# ═══════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    app = MoxHubApp()
    app.mainloop()
