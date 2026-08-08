#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════
#  Mox HUB — Binary Engine Manager & Project Launcher v3.0
#  Unity Hub / Epic Games Launcher Style (Pure Binary Manager)
# ═══════════════════════════════════════════════════════════════════

import os
import sys
import json
import time
import io
import queue
import shutil
import zipfile
import tarfile
import tempfile
import threading
import subprocess
import urllib.request
import urllib.error
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from datetime import datetime

# ─── GitHub Release Direct ZIP URL ─────────────────────────────────
GITHUB_RELEASE_URL = "https://github.com/YOUR_USERNAME/YOUR_REPO/releases/download/v1.0.0/MoxEngine.zip"

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

# ─── Design Tokens ─────────────────────────────────────────────────
C = {
    "bg":           "#1e1e24",
    "sidebar":      "#16161c",
    "card":         "#2b2d3a",
    "card_hover":   "#343648",
    "border":       "#383a50",
    "accent":       "#7c9fe6",
    "accent2":      "#a78bfa",
    "success":      "#4ade80",
    "warning":      "#fb923c",
    "danger":       "#f87171",
    "fg":           "#e2e4f0",
    "fg2":          "#8b8fa8",
    "fg3":          "#5a5c70",
    "topbar":       "#13131a",
    "btn_primary":  "#5b7fd4",
    "btn_success":  "#22c55e",
    "btn_danger":   "#dc2626",
    "sidebar_sel":  "#2b2d3a",
    "sidebar_w":    220,
    "topbar_h":     56,
}

# ─── Project Templates ─────────────────────────────────────────────
TEMPLATES = [
    {
        "id": "demo",
        "name": "Mox Interactive Demo",
        "icon": "⭐",
        "desc": "Full Mox Engine demo with PlayerNode, Camera Shake, Particles, and NovaScript ScriptedBox.",
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
        "desc": "Minimal setup. Empty main.cpp, CMakeLists.txt, and scripts/ folder ready.",
        "tag":  "Blank",
    },
]

# ─── Default Engine Releases Catalogue ──────────────────────────────
_DEFAULT_RELEASES = [
    {
        "ver":    "Mox Engine1",
        "label":  "Mox Engine1 (Core Build)",
        "status": "installed",
        "date":   datetime.now().strftime("%Y-%m-%d"),
        "url":    None,
        "desc":   "Pre-compiled core Mox Engine binary with DirectX 11 & OpenGL hardware acceleration.",
        "path":   os.path.join(VERSIONS_DIR, "Mox Engine1", "MoxEngine.exe"),
    },
]


# ═══════════════════════════════════════════════════════════════════
class MoxHubApp(tk.Tk):

    def __init__(self):
        super().__init__()
        self.title("Mox HUB")
        self.geometry("1180x720")
        self.minsize(900, 600)
        self.configure(bg=C["bg"])

        self.active_nav     = tk.StringVar(value="projects")
        self.projects       = []
        self.engine_versions= []
        self.active_version = "Mox Engine1"
        self.sel_template   = tk.StringVar(value=TEMPLATES[0]["id"])
        self.log_queue      = queue.Queue()

        self._ensure_dirs()
        self._auto_package_mox_engine()
        self._load_data()
        self._scan_installed_versions()
        self._build_layout()
        self._switch_nav("projects")
        self._update_status()

    # ─── Setup & Auto-Packaging ───────────────────────────────────
    def _ensure_dirs(self):
        os.makedirs(PROJECTS_DIR, exist_ok=True)
        os.makedirs(VERSIONS_DIR, exist_ok=True)

    def _auto_package_mox_engine(self):
        """Automatically packages local C++ executable into Engine/versions/Mox Engine1/MoxEngine.exe."""
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

        # Copy local build folder contents if available
        if os.path.exists(local_build):
            try:
                for item in os.listdir(local_build):
                    s = os.path.join(local_build, item)
                    d = os.path.join(target_dir, item)
                    if os.path.isdir(s):
                        shutil.copytree(s, d, dirs_exist_ok=True)
                    else:
                        shutil.copy2(s, d)
            except Exception:
                pass

        if found_src and not os.path.exists(target_exe):
            try:
                shutil.copy2(found_src, target_exe)
            except Exception:
                pass

    def _load_data(self):
        # Load Projects
        if os.path.exists(PROJECTS_DB):
            try:
                with open(PROJECTS_DB, "r", encoding="utf-8") as f:
                    self.projects = json.load(f)
            except Exception:
                self.projects = []
        else:
            self.projects = []

        # Load Versions Catalogue — enforce single-version (Mox Engine1 only)
        self.engine_versions = list(_DEFAULT_RELEASES)
        self.active_version  = "Mox Engine1"

        if os.path.exists(VERSIONS_DB):
            try:
                with open(VERSIONS_DB, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    # Pull only the Mox Engine1 entry if it was previously saved
                    saved = next(
                        (v for v in data.get("versions", []) if v.get("ver") == "Mox Engine1"),
                        None
                    )
                    if saved:
                        self.engine_versions = [saved]
            except Exception:
                pass

        # Always reset active to Mox Engine1
        self.active_version = "Mox Engine1"

    def _save_data(self):
        with open(PROJECTS_DB, "w", encoding="utf-8") as f:
            json.dump(self.projects, f, ensure_ascii=False, indent=2)

        save_ver = []
        for v in self.engine_versions:
            item = {
                "ver":    v["ver"],
                "label":  v["label"],
                "status": v["status"],
                "date":   v["date"],
                "url":    v.get("url"),
                "desc":   v.get("desc", ""),
                "path":   v.get("path", ""),
            }
            save_ver.append(item)

        with open(VERSIONS_DB, "w", encoding="utf-8") as f:
            json.dump({"active_version": self.active_version, "versions": save_ver},
                      f, ensure_ascii=False, indent=2)

    def _scan_installed_versions(self):
        """Checks Engine/versions/Mox Engine1/ for the installed MoxEngine binary."""
        mox1_path = os.path.join(VERSIONS_DIR, "Mox Engine1")
        mox1_exe  = self._find_exe_in_dir(mox1_path)

        # There is only one version: Mox Engine1
        self.engine_versions = [{
            "ver":    "Mox Engine1",
            "label":  "Mox Engine1 (Core Build)",
            "status": "installed" if mox1_exe else "not_installed",
            "date":   datetime.now().strftime("%Y-%m-%d"),
            "url":    None,
            "desc":   "Pre-compiled core Mox Engine binary — the one and only official release.",
            "path":   mox1_exe or mox1_path,
        }]
        self.active_version = "Mox Engine1"

    def _find_exe_in_dir(self, directory):
        """Recursively search for MoxEngine.exe, Nova2D.exe, or any .exe inside version directory."""
        if not directory or not os.path.exists(directory):
            return None
        if os.path.isfile(directory) and directory.lower().endswith(".exe"):
            return directory

        # 1. Direct check MoxEngine.exe
        mox_exe = os.path.join(directory, "MoxEngine.exe")
        if os.path.exists(mox_exe):
            return mox_exe

        # 2. Direct check Nova2D.exe
        nova_exe = os.path.join(directory, "Nova2D.exe")
        if os.path.exists(nova_exe):
            return nova_exe

        # 3. Check build subfolder
        for sub in ("MoxEngine.exe", "Nova2D.exe"):
            build_exe = os.path.join(directory, "build", sub)
            if os.path.exists(build_exe):
                return build_exe

        # 4. Recursive search for any .exe
        for root, dirs, files in os.walk(directory):
            for file in files:
                if file.lower().endswith(".exe"):
                    return os.path.join(root, file)
        return None

    def _get_active_exe(self):
        """Returns the executable path for the currently selected engine version with full fallbacks."""
        # 1. Check Mox Engine1 directory specifically
        mox1_exe = self._find_exe_in_dir(os.path.join(VERSIONS_DIR, "Mox Engine1"))
        if mox1_exe:
            mox1_entry = next((v for v in self.engine_versions if v["ver"] == "Mox Engine1"), None)
            if mox1_entry:
                mox1_entry["status"] = "installed"
                mox1_entry["path"]   = mox1_exe
            if self.active_version == "Mox Engine1":
                return mox1_exe

        # 2. Search in active version record
        for v in self.engine_versions:
            if v["ver"] == self.active_version:
                exe_path = v.get("path")
                if exe_path:
                    found = self._find_exe_in_dir(exe_path)
                    if found:
                        v["status"] = "installed"
                        v["path"]   = found
                        return found

        # 3. Check Engine/versions/ subfolders
        candidate_dirs = [
            os.path.join(VERSIONS_DIR, self.active_version),
            os.path.join(VERSIONS_DIR, f"v{self.active_version}"),
            VERSIONS_DIR,
        ]
        for cdir in candidate_dirs:
            found = self._find_exe_in_dir(cdir)
            if found:
                return found

        # 4. Fallback to local Engine/build/ or Engine/ root
        for name in ("MoxEngine.exe", "Nova2D.exe"):
            p1 = os.path.join(ENGINE_DIR, "build", name)
            if os.path.exists(p1):
                return p1
            p2 = os.path.join(ENGINE_DIR, name)
            if os.path.exists(p2):
                return p2

        return None

    # ─── Root Layout ───────────────────────────────────────────────
    def _build_layout(self):
        # Top bar
        self.topbar = tk.Frame(self, bg=C["topbar"], height=C["topbar_h"])
        self.topbar.pack(side="top", fill="x")
        self.topbar.pack_propagate(False)
        self._build_topbar()

        # Main Body = Sidebar + Content
        body = tk.Frame(self, bg=C["bg"])
        body.pack(side="top", fill="both", expand=True)

        self.sidebar = tk.Frame(body, bg=C["sidebar"], width=C["sidebar_w"])
        self.sidebar.pack(side="left", fill="y")
        self.sidebar.pack_propagate(False)
        self._build_sidebar()

        self.main_frame = tk.Frame(body, bg=C["bg"])
        self.main_frame.pack(side="left", fill="both", expand=True)

        # Pages
        self.pages = {}
        for key in ("projects", "installs", "settings"):
            page = tk.Frame(self.main_frame, bg=C["bg"])
            self.pages[key] = page

        self._build_page_projects()
        self._build_page_installs()
        self._build_page_settings()

    # ─── Top Bar ───────────────────────────────────────────────────
    def _build_topbar(self):
        # Logo
        tk.Label(self.topbar, text="  ⚡ Mox HUB",
                 font=("Segoe UI", 14, "bold"),
                 bg=C["topbar"], fg=C["accent"]).pack(side="left", padx=(12, 0))

        # Right-side cluster
        right = tk.Frame(self.topbar, bg=C["topbar"])
        right.pack(side="right", padx=14)

        # Active Version badge
        self.lbl_version = tk.Label(right, text=f"{self.active_version}",
                                    font=("Segoe UI", 9, "bold"),
                                    bg="#2b3a5a", fg=C["accent"],
                                    padx=10, pady=3, relief="flat")
        self.lbl_version.pack(side="left", padx=(0, 12))

        # Status badge
        self.lbl_status = tk.Label(right, text="● MOX ENGINE READY",
                                   font=("Segoe UI", 9, "bold"),
                                   bg=C["topbar"], fg=C["success"])
        self.lbl_status.pack(side="left", padx=(0, 16))

        # Launch Engine button
        self.btn_launch = self._mk_btn(right, "▶  Launch Mox Engine",
                                       self.launch_engine,
                                       bg=C["btn_success"], fg="#0f2318",
                                       font=("Segoe UI", 10, "bold"), padx=18, pady=6)
        self.btn_launch.pack(side="left")

    # ─── Sidebar ───────────────────────────────────────────────────
    def _build_sidebar(self):
        tk.Frame(self.sidebar, bg=C["accent"], height=2).pack(fill="x")

        tk.Label(self.sidebar, text="MOX HUB",
                 font=("Segoe UI", 8, "bold"),
                 bg=C["sidebar"], fg=C["fg3"]).pack(pady=(16, 12))

        nav_items = [
            ("projects",  "🚀", "Projects"),
            ("installs",  "🛠️", "Installs"),
            ("settings",  "⚙️", "Settings"),
        ]
        self.nav_btns = {}
        for key, icon, label in nav_items:
            btn = self._mk_nav_btn(key, icon, label)
            self.nav_btns[key] = btn

        spacer = tk.Frame(self.sidebar, bg=C["sidebar"])
        spacer.pack(fill="both", expand=True)

        footer = tk.Frame(self.sidebar, bg=C["sidebar"], padx=14, pady=12)
        footer.pack(fill="x", side="bottom")
        tk.Label(footer, text="Mox Engine Hub",
                 font=("Segoe UI", 8, "bold"), bg=C["sidebar"], fg=C["fg2"]).pack(anchor="w")
        tk.Label(footer, text="Mox Engine1  |  Standalone",
                 font=("Segoe UI", 8), bg=C["sidebar"], fg=C["fg3"]).pack(anchor="w")

    def _mk_nav_btn(self, key, icon, label):
        frame = tk.Frame(self.sidebar, bg=C["sidebar"], cursor="hand2")
        frame.pack(fill="x", padx=8, pady=3)

        inner = tk.Frame(frame, bg=C["sidebar"], pady=10, padx=14)
        inner.pack(fill="x")

        icon_lbl = tk.Label(inner, text=icon, font=("Segoe UI", 14),
                            bg=C["sidebar"], fg=C["fg2"])
        icon_lbl.pack(side="left")

        text_lbl = tk.Label(inner, text=f"  {label}", font=("Segoe UI", 10, "bold"),
                            bg=C["sidebar"], fg=C["fg2"])
        text_lbl.pack(side="left")

        for w in (frame, inner, icon_lbl, text_lbl):
            w.bind("<Button-1>", lambda e, k=key: self._switch_nav(k))
            w.bind("<Enter>",    lambda e, f=inner, il=icon_lbl, tl=text_lbl: self._nav_hover(f, il, tl, True))
            w.bind("<Leave>",    lambda e, k=key, f=inner, il=icon_lbl, tl=text_lbl: self._nav_hover(f, il, tl, False, k))

        return (frame, inner, icon_lbl, text_lbl)

    def _nav_hover(self, frame, icon_lbl, text_lbl, enter, key=None):
        if key and self.active_nav.get() == key:
            return
        bg = C["card_hover"] if enter else C["sidebar"]
        fg = C["fg"] if enter else C["fg2"]
        for w in (frame, icon_lbl, text_lbl):
            w.configure(bg=bg)
        text_lbl.configure(fg=fg)

    def _switch_nav(self, key):
        prev = self.active_nav.get()
        self.active_nav.set(key)

        if prev in self.nav_btns:
            frame, inner, icon_lbl, text_lbl = self.nav_btns[prev]
            for w in (frame, inner, icon_lbl, text_lbl):
                w.configure(bg=C["sidebar"])
            text_lbl.configure(fg=C["fg2"])

        frame, inner, icon_lbl, text_lbl = self.nav_btns[key]
        for w in (frame, inner, icon_lbl, text_lbl):
            w.configure(bg=C["sidebar_sel"])
        text_lbl.configure(fg=C["accent"])
        icon_lbl.configure(fg=C["accent"])

        for k, p in self.pages.items():
            p.pack_forget()
        self.pages[key].pack(fill="both", expand=True)

    # ═══════════════════════════════════════════════════════════════
    # PAGE: Projects
    # ═══════════════════════════════════════════════════════════════
    def _build_page_projects(self):
        page = self.pages["projects"]

        hdr = tk.Frame(page, bg=C["bg"], padx=28, pady=20)
        hdr.pack(fill="x")

        tk.Label(hdr, text="Projects", font=("Segoe UI", 20, "bold"),
                 bg=C["bg"], fg=C["fg"]).pack(side="left")

        btn_row = tk.Frame(hdr, bg=C["bg"])
        btn_row.pack(side="right")

        self._mk_btn(btn_row, "+ New Project", self._open_new_project_dialog,
                     bg=C["btn_primary"], fg="#fff", padx=14, pady=6).pack(side="left", padx=5)
        self._mk_btn(btn_row, "📂 Open Project", self._open_existing_project,
                     bg=C["card"], fg=C["fg"], padx=14, pady=6).pack(side="left")

        self.projects_list_frame = tk.Frame(page, bg=C["bg"], padx=24)
        self.projects_list_frame.pack(fill="both", expand=True)
        self._render_project_list()

    def _render_project_list(self):
        for w in self.projects_list_frame.winfo_children():
            w.destroy()

        if not self.projects:
            empty = tk.Frame(self.projects_list_frame, bg=C["bg"])
            empty.pack(expand=True, fill="both", pady=80)
            tk.Label(empty, text="🚀", font=("Segoe UI", 48),
                     bg=C["bg"], fg=C["fg3"]).pack()
            tk.Label(empty, text="No projects found",
                     font=("Segoe UI", 16, "bold"), bg=C["bg"], fg=C["fg2"]).pack(pady=6)
            tk.Label(empty, text="Click '+ New Project' to create your first Mox Engine game",
                     font=("Segoe UI", 10), bg=C["bg"], fg=C["fg3"]).pack()
            return

        hdr = tk.Frame(self.projects_list_frame, bg=C["bg"], pady=4)
        hdr.pack(fill="x")
        for txt, anchor in [("Project Name", "w"), ("Target Engine", "center"), ("Last Opened", "center"), ("Actions", "e")]:
            tk.Label(hdr, text=txt, font=("Segoe UI", 9, "bold"),
                     bg=C["bg"], fg=C["fg3"], anchor=anchor).pack(side="left", expand=True, fill="x")

        tk.Frame(self.projects_list_frame, bg=C["border"], height=1).pack(fill="x", pady=(0, 6))

        for proj in reversed(self.projects):
            self._render_project_card(proj)

    def _render_project_card(self, proj):
        card = tk.Frame(self.projects_list_frame, bg=C["card"],
                        pady=12, padx=16, cursor="hand2")
        card.pack(fill="x", pady=3)

        left = tk.Frame(card, bg=C["card"])
        left.pack(side="left", fill="x", expand=True)

        name_row = tk.Frame(left, bg=C["card"])
        name_row.pack(anchor="w")
        tk.Label(name_row, text=proj.get("icon", "🎮"),
                 font=("Segoe UI", 14), bg=C["card"], fg=C["fg"]).pack(side="left")
        tk.Label(name_row, text=f"  {proj['name']}",
                 font=("Segoe UI", 11, "bold"), bg=C["card"], fg=C["fg"]).pack(side="left")
        tk.Label(left, text=proj.get("path", ""),
                 font=("Consolas", 8), bg=C["card"], fg=C["fg3"]).pack(anchor="w", padx=(2, 0))

        tk.Label(card, text=f"{proj.get('engine_ver', self.active_version)}",
                 font=("Segoe UI", 9, "bold"), bg="#1e2a45", fg=C["accent"],
                 padx=8, pady=2).pack(side="left", padx=20)

        tk.Label(card, text=proj.get("last_opened", "Never"),
                 font=("Segoe UI", 9), bg=C["card"], fg=C["fg2"]).pack(side="left", padx=20)

        btn_row = tk.Frame(card, bg=C["card"])
        btn_row.pack(side="right")

        self._mk_btn(btn_row, "▶ Launch", lambda p=proj: self._launch_project(p),
                     bg=C["btn_success"], fg="#0f2318", padx=10, pady=4,
                     font=("Segoe UI", 9, "bold")).pack(side="left", padx=3)
        self._mk_btn(btn_row, "VS Code", lambda p=proj: self._open_vscode(p),
                     bg=C["card_hover"], fg=C["fg"], padx=10, pady=4,
                     font=("Segoe UI", 9)).pack(side="left", padx=3)
        self._mk_btn(btn_row, "✕", lambda p=proj: self._remove_project(p),
                     bg=C["card"], fg=C["fg3"], padx=8, pady=4,
                     font=("Segoe UI", 9)).pack(side="left")

        for w in card.winfo_children() + [card]:
            w.bind("<Enter>", lambda e, c=card: c.configure(bg=C["card_hover"]))
            w.bind("<Leave>", lambda e, c=card: c.configure(bg=C["card"]))

    def _open_new_project_dialog(self):
        win = tk.Toplevel(self)
        win.title("New Mox Engine Project")
        win.geometry("600x480")
        win.configure(bg=C["bg"])
        win.resizable(False, False)
        win.grab_set()

        tk.Label(win, text="Create New Project",
                 font=("Segoe UI", 15, "bold"), bg=C["bg"], fg=C["fg"]).pack(pady=(20, 4), padx=24, anchor="w")
        tk.Label(win, text="Select a template and destination folder.",
                 font=("Segoe UI", 9), bg=C["bg"], fg=C["fg2"]).pack(padx=24, anchor="w", pady=(0, 16))

        tpl_frame = tk.Frame(win, bg=C["bg"], padx=20)
        tpl_frame.pack(fill="x")
        for tpl in TEMPLATES:
            rb_card = tk.Frame(tpl_frame, bg=C["card"], padx=12, pady=8, cursor="hand2")
            rb_card.pack(fill="x", pady=3)
            tk.Radiobutton(rb_card, variable=self.sel_template, value=tpl["id"],
                           bg=C["card"], activebackground=C["card"],
                           selectcolor=C["accent"]).pack(side="left")
            tk.Label(rb_card, text=f"{tpl['icon']}  {tpl['name']}",
                     font=("Segoe UI", 10, "bold"), bg=C["card"], fg=C["fg"]).pack(side="left")
            tk.Label(rb_card, text=tpl["desc"],
                     font=("Segoe UI", 8), bg=C["card"], fg=C["fg2"]).pack(side="left", padx=8)

        form = tk.Frame(win, bg=C["bg"], padx=24, pady=12)
        form.pack(fill="x")
        tk.Label(form, text="Project Name", font=("Segoe UI", 10, "bold"),
                 bg=C["bg"], fg=C["fg2"]).pack(anchor="w")
        name_var = tk.StringVar(value="MyMoxGame")
        name_entry = tk.Entry(form, textvariable=name_var, font=("Segoe UI", 11),
                              bg=C["card"], fg=C["fg"], insertbackground=C["fg"],
                              relief="flat", bd=8)
        name_entry.pack(fill="x", pady=(4, 10))

        tk.Label(form, text="Location", font=("Segoe UI", 10, "bold"),
                 bg=C["bg"], fg=C["fg2"]).pack(anchor="w")
        loc_var = tk.StringVar(value=PROJECTS_DIR)
        loc_row = tk.Frame(form, bg=C["bg"])
        loc_row.pack(fill="x", pady=(4, 0))
        tk.Entry(loc_row, textvariable=loc_var, font=("Segoe UI", 10),
                 bg=C["card"], fg=C["fg"], insertbackground=C["fg"],
                 relief="flat", bd=6).pack(side="left", fill="x", expand=True)
        self._mk_btn(loc_row, "Browse",
                     lambda: loc_var.set(filedialog.askdirectory(initialdir=PROJECTS_DIR) or loc_var.get()),
                     bg=C["card_hover"], fg=C["fg"], padx=10, pady=4).pack(side="left", padx=(4, 0))

        bot = tk.Frame(win, bg=C["bg"], padx=24, pady=16)
        bot.pack(fill="x", side="bottom")
        self._mk_btn(bot, "Cancel", win.destroy,
                     bg=C["card"], fg=C["fg"], padx=14, pady=6).pack(side="right", padx=6)
        self._mk_btn(bot, "Create Project",
                     lambda: self._create_project(name_var.get(), loc_var.get(), win),
                     bg=C["btn_primary"], fg="#fff", padx=14, pady=6,
                     font=("Segoe UI", 10, "bold")).pack(side="right")

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
        if not path:
            return
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

    def _launch_project(self, proj):
        exe_path = self._get_active_exe()
        if not exe_path or not os.path.exists(exe_path):
            messagebox.showwarning("Engine Required",
                f"No binary executable found for {self.active_version}.\n\n"
                "Please go to the 'Installs' tab and click 'Download & Install'.")
            return

        try:
            work_dir = os.path.dirname(exe_path)
            subprocess.Popen([exe_path], cwd=work_dir)
            proj["last_opened"] = datetime.now().strftime("%Y-%m-%d %H:%M")
            self._save_data()
            self._render_project_list()
        except Exception as e:
            messagebox.showerror("Launch Error", f"Could not launch executable:\n{e}")

    def _open_vscode(self, proj):
        path = proj.get("path", "")
        try:
            subprocess.Popen(["code", path], shell=True)
        except Exception as e:
            messagebox.showerror("Error", f"Could not open VS Code:\n{e}")

    def _remove_project(self, proj):
        if messagebox.askyesno("Remove", f"Remove '{proj['name']}' from list?\n(Project files on disk will NOT be deleted)"):
            self.projects = [p for p in self.projects if p["path"] != proj["path"]]
            self._save_data()
            self._render_project_list()

    # ═══════════════════════════════════════════════════════════════
    # PAGE: Installs / Mox Engine Versions
    # ═══════════════════════════════════════════════════════════════
    def _build_page_installs(self):
        page = self.pages["installs"]

        hdr = tk.Frame(page, bg=C["bg"], padx=28, pady=20)
        hdr.pack(fill="x")
        tk.Label(hdr, text="Mox Engine Installs", font=("Segoe UI", 20, "bold"),
                 bg=C["bg"], fg=C["fg"]).pack(side="left")

        btn_row = tk.Frame(hdr, bg=C["bg"])
        btn_row.pack(side="right")
        self._mk_btn(btn_row, "↻ Refresh", self._refresh_versions,
                     bg=C["card"], fg=C["fg2"], padx=10, pady=6).pack(side="left", padx=3)
        self._mk_btn(btn_row, "📂 Install from Local ZIP / Folder", self._install_local_dialog,
                     bg=C["card_hover"], fg=C["fg"], padx=12, pady=6,
                     font=("Segoe UI", 9)).pack(side="left", padx=3)
        self._mk_btn(btn_row, "📦 Package Local Build", self._bundle_local_build_dialog,
                     bg=C["btn_primary"], fg="#fff", padx=12, pady=6,
                     font=("Segoe UI", 9, "bold")).pack(side="left", padx=3)

        sub = tk.Frame(page, bg=C["bg"], padx=28)
        sub.pack(fill="x", pady=(0, 10))
        tk.Label(sub, text="Manage official Mox Engine pre-compiled binaries and standalone releases.",
                 font=("Segoe UI", 9), bg=C["bg"], fg=C["fg2"]).pack(anchor="w")

        self.ver_list_frame = tk.Frame(page, bg=C["bg"], padx=28)
        self.ver_list_frame.pack(fill="both", expand=True)
        self._render_version_list()

    def _refresh_versions(self):
        self._scan_installed_versions()
        self._save_data()
        self._render_version_list()
        self._update_status()

    def _render_version_list(self):
        for w in self.ver_list_frame.winfo_children():
            w.destroy()

        for ver_info in self.engine_versions:
            self._render_version_card(ver_info)

    def _render_version_card(self, ver_info):
        is_installed  = ver_info.get("status") == "installed"
        is_active     = ver_info["ver"] == self.active_version
        is_loading    = ver_info.get("status") == "downloading"
        card = tk.Frame(self.ver_list_frame, bg=C["card"], padx=18, pady=14)
        card.pack(fill="x", pady=4)

        left = tk.Frame(card, bg=C["card"])
        left.pack(side="left", expand=True, fill="x")

        name_row = tk.Frame(left, bg=C["card"])
        name_row.pack(anchor="w")
        tk.Label(name_row, text=ver_info["label"], font=("Segoe UI", 11, "bold"),
                 bg=C["card"], fg=C["fg"]).pack(side="left")
        if is_active:
            tk.Label(name_row, text="  DEFAULT ACTIVE", font=("Segoe UI", 8, "bold"),
                     bg="#1c3a1c", fg=C["success"], padx=8, pady=1).pack(side="left", padx=8)

        desc_lbl = tk.Label(left, text=ver_info.get("desc", f"Mox Engine release {ver_info['ver']}"),
                            font=("Segoe UI", 8), bg=C["card"], fg=C["fg2"])
        desc_lbl.pack(anchor="w", pady=(2, 0))

        if is_loading:
            prog_frame = tk.Frame(left, bg=C["card"])
            prog_frame.pack(fill="x", pady=(8, 0))
            pbar = ttk.Progressbar(prog_frame, mode="determinate", length=340)
            pbar.pack(side="left")
            speed_lbl = tk.Label(prog_frame, text="Starting download...",
                                 font=("Consolas", 8), bg=C["card"], fg=C["accent"])
            speed_lbl.pack(side="left", padx=10)
            ver_info["_pbar"]      = pbar
            ver_info["_speed_lbl"] = speed_lbl

        if is_installed:
            status_txt, status_color = "● Installed", C["success"]
        else:
            status_txt, status_color = "◎ Not Installed", C["warning"]

        tk.Label(card, text=status_txt, font=("Segoe UI", 9, "bold"),
                 bg=C["card"], fg=status_color).pack(side="left", padx=24)

        btn_row = tk.Frame(card, bg=C["card"])
        btn_row.pack(side="right")

        if is_installed:
            self._mk_btn(btn_row, "▶ Test Run",
                         lambda v=ver_info: self._test_run_version(v),
                         bg=C["card_hover"], fg=C["fg"], padx=10, pady=5,
                         font=("Segoe UI", 9)).pack(side="left", padx=4)
            self._mk_btn(btn_row, "📦 Re-Package Build",
                         lambda: self._bundle_local_build_dialog(),
                         bg=C["card"], fg=C["accent"], padx=10, pady=5,
                         font=("Segoe UI", 9)).pack(side="left", padx=4)
        else:
            self._mk_btn(btn_row, "⬇ Download & Install Mox Engine",
                         lambda: self._install_remote_engine(),
                         bg=C["btn_success"], fg="#0f2318", padx=16, pady=7,
                         font=("Segoe UI", 10, "bold")).pack(side="left", padx=4)
            self._mk_btn(btn_row, "📦 Package Local Build",
                         lambda: self._bundle_local_build_dialog(),
                         bg=C["card_hover"], fg=C["fg"], padx=10, pady=5,
                         font=("Segoe UI", 9)).pack(side="left", padx=4)
            self._mk_btn(btn_row, "📂 Install from Local ZIP",
                         lambda: self._install_local_dialog(),
                         bg=C["card"], fg=C["fg3"], padx=10, pady=5,
                         font=("Segoe UI", 9)).pack(side="left")

    def _set_default_version(self, ver):
        self.active_version = ver
        self.lbl_version.configure(text=f"{ver}")
        self._save_data()
        self._render_version_list()
        self._update_status()

    # ─── (Online download removed — Mox Engine1 uses local packaging only) ──

    def _test_run_version(self, ver_info):
        path = ver_info.get("path")
        exe  = self._find_exe_in_dir(path) if (path and os.path.isdir(path)) else path
        if not exe or not os.path.exists(exe):
            exe = os.path.join(VERSIONS_DIR, "Mox Engine1", "MoxEngine.exe")

        if not exe or not os.path.exists(exe):
            messagebox.showwarning("Executable Not Found", f"No MoxEngine.exe found in:\n{path}")
            return

        try:
            subprocess.Popen([exe], cwd=os.path.dirname(exe))
        except Exception as e:
            messagebox.showerror("Error", f"Failed to launch:\n{e}")

    def _remove_version(self, ver_info):
        if not messagebox.askyesno("Uninstall Version",
                                   f"Uninstall {ver_info['label']}?\n\nDirectory will be deleted."):
            return

        path = ver_info.get("path", "")
        if path and os.path.exists(path):
            if os.path.isfile(path):
                parent = os.path.dirname(path)
                if parent.startswith(VERSIONS_DIR):
                    shutil.rmtree(parent, ignore_errors=True)
            else:
                shutil.rmtree(path, ignore_errors=True)

        ver_info["status"] = "available"
        ver_info["path"]   = ""
        self._save_data()
        self._render_version_list()
        self._update_status()

    # ─── (Error/fallback popup removed — use local packaging only) ──

    # ─── Local File & Folder Installation ─────────────────────────
    def _install_local_dialog(self, default_ver="Mox Engine1"):
        win = tk.Toplevel(self)
        win.title("Install Engine from Local File / ZIP")
        win.geometry("580x320")
        win.configure(bg=C["bg"])
        win.resizable(False, False)
        win.grab_set()

        tk.Label(win, text="Install Mox Engine from Local Package",
                 font=("Segoe UI", 14, "bold"), bg=C["bg"], fg=C["fg"]).pack(
                 pady=(18, 4), padx=24, anchor="w")
        tk.Label(win, text="Select a local .zip archive or existing engine directory.",
                 font=("Segoe UI", 9), bg=C["bg"], fg=C["fg2"]).pack(padx=24, anchor="w")

        form = tk.Frame(win, bg=C["bg"], padx=24, pady=12)
        form.pack(fill="x")

        tk.Label(form, text="Version Name (e.g. Mox Engine1)",
                 font=("Segoe UI", 9, "bold"), bg=C["bg"], fg=C["fg2"]).pack(anchor="w")
        ver_var = tk.StringVar(value=default_ver)
        tk.Entry(form, textvariable=ver_var, font=("Segoe UI", 10),
                 bg=C["card"], fg=C["fg"], insertbackground=C["fg"],
                 relief="flat", bd=6).pack(fill="x", pady=(4, 10))

        tk.Label(form, text="Local Source Path (.zip file or folder)",
                 font=("Segoe UI", 9, "bold"), bg=C["bg"], fg=C["fg2"]).pack(anchor="w")
        path_var = tk.StringVar()
        p_row = tk.Frame(form, bg=C["bg"])
        p_row.pack(fill="x", pady=(4, 0))
        tk.Entry(p_row, textvariable=path_var, font=("Consolas", 9),
                 bg=C["card"], fg=C["fg"], insertbackground=C["fg"],
                 relief="flat", bd=6).pack(side="left", fill="x", expand=True)

        def _pick_file():
            f = filedialog.askopenfilename(filetypes=[("ZIP / Archive", "*.zip;*.tar.gz;*.rar"), ("All files", "*.*")])
            if f:
                path_var.set(f)

        def _pick_dir():
            d = filedialog.askdirectory(initialdir=PROJECT_ROOT, title="Select Local Engine Folder")
            if d:
                path_var.set(d)

        self._mk_btn(p_row, "ZIP...", _pick_file, bg=C["card_hover"], fg=C["fg"], padx=8, pady=4).pack(side="left", padx=3)
        self._mk_btn(p_row, "Folder...", _pick_dir, bg=C["card_hover"], fg=C["fg"], padx=8, pady=4).pack(side="left")

        def _do_install():
            ver = ver_var.get().strip()
            src = path_var.get().strip()
            if not ver or not src:
                messagebox.showwarning("Missing Information", "Please enter version name and select a source path.", parent=win)
                return
            if not os.path.exists(src):
                messagebox.showerror("Error", f"Path does not exist:\n{src}", parent=win)
                return

            target_dir = os.path.join(VERSIONS_DIR, ver)
            os.makedirs(target_dir, exist_ok=True)

            try:
                if os.path.isfile(src) and zipfile.is_zipfile(src):
                    with zipfile.ZipFile(src, "r") as zf:
                        zf.extractall(target_dir)
                elif os.path.isdir(src):
                    for item in os.listdir(src):
                        s = os.path.join(src, item)
                        d = os.path.join(target_dir, item)
                        if os.path.isdir(s):
                            shutil.copytree(s, d, dirs_exist_ok=True)
                        else:
                            shutil.copy2(s, d)

                exe_found = self._find_exe_in_dir(target_dir)

                existing = next((v for v in self.engine_versions if v["ver"] == ver), None)
                if existing:
                    existing["status"] = "installed"
                    existing["path"]   = exe_found or target_dir
                else:
                    self.engine_versions.append({
                        "ver":    ver,
                        "label":  f"{ver} (Local Install)",
                        "status": "installed",
                        "date":   datetime.now().strftime("%Y-%m-%d"),
                        "url":    None,
                        "desc":   f"Installed from local package: {os.path.basename(src)}",
                        "path":   exe_found or target_dir,
                    })

                self.active_version = ver
                self._save_data()
                win.destroy()
                self._render_version_list()
                self._update_status()
                messagebox.showinfo("Success", f"{ver} installed successfully from local package!")
            except Exception as e:
                messagebox.showerror("Installation Error", f"Could not install local package:\n{e}", parent=win)

        bot = tk.Frame(win, bg=C["bg"], padx=24, pady=12)
        bot.pack(fill="x", side="bottom")
        self._mk_btn(bot, "Cancel", win.destroy, bg=C["card"], fg=C["fg"], padx=12, pady=6).pack(side="right", padx=6)
        self._mk_btn(bot, "✓ Install Package", _do_install,
                     bg=C["btn_primary"], fg="#fff", padx=14, pady=6,
                     font=("Segoe UI", 10, "bold")).pack(side="right")

    # ─── Local Build Bundling ──────────────────────────────────────
    def _bundle_local_build_dialog(self):
        """Directly copy local engine binary into Engine/versions/Mox Engine1/ — no popup, single click."""
        ver        = "Mox Engine1"
        target_dir = os.path.join(VERSIONS_DIR, ver)
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

        try:
            # Copy everything from Engine/build/ into the version folder
            if os.path.exists(local_build):
                for item in os.listdir(local_build):
                    s = os.path.join(local_build, item)
                    d = os.path.join(target_dir, item)
                    if os.path.isdir(s):
                        shutil.copytree(s, d, dirs_exist_ok=True)
                    else:
                        shutil.copy2(s, d)

            # Always force-copy the main executable
            if found_src:
                shutil.copy2(found_src, target_exe)

            final_exe = target_exe if os.path.exists(target_exe) else self._find_exe_in_dir(target_dir)

            # Update the single Mox Engine1 entry immediately
            entry = next((x for x in self.engine_versions if x["ver"] == ver), None)
            if entry:
                entry["status"] = "installed" if final_exe else "not_installed"
                entry["path"]   = final_exe or target_dir

            self.active_version = ver
            self._save_data()
            self._render_version_list()
            self._update_status()

            if final_exe and os.path.exists(final_exe):
                messagebox.showinfo(
                    "✅ Mox Engine1 Ready",
                    f"Engine packaged successfully!\n\nBinary: {final_exe}\n\nTop bar now shows: ● MOX ENGINE READY"
                )
            else:
                messagebox.showwarning(
                    "No Binary Found",
                    "No MoxEngine.exe or Nova2D.exe was found in Engine/build/.\n\n"
                    "Please build the engine first, or use 'Install from Local ZIP / Folder' to point to an existing binary."
                )
        except Exception as e:
            messagebox.showerror("Bundle Error", f"Could not package local build:\n{e}")

    # ─── (URL download dialog removed — Mox Engine1 is locally packaged only) ──

    # ─── Remote Release Downloader & Installer ──────────────────────
    def _install_remote_engine(self):
        """Download pre-compiled Mox Engine zip directly from GitHub Release URL and extract."""
        target_dir = os.path.join(VERSIONS_DIR, "Mox Engine1")
        os.makedirs(target_dir, exist_ok=True)

        # ─── Progress Dialog ─────────────────────────────────────
        win = tk.Toplevel(self)
        win.title("Downloading Mox Engine1...")
        win.geometry("500x220")
        win.configure(bg=C["bg"])
        win.resizable(False, False)
        win.grab_set()

        tk.Label(win, text="⬇  Downloading Mox Engine1 Release",
                 font=("Segoe UI", 13, "bold"), bg=C["bg"], fg=C["fg"]).pack(
                 pady=(20, 4), padx=24, anchor="w")

        status_var = tk.StringVar(value="Connecting to GitHub Release...")
        tk.Label(win, textvariable=status_var,
                 font=("Segoe UI", 9), bg=C["bg"], fg=C["fg2"]).pack(padx=24, anchor="w")

        pbar = ttk.Progressbar(win, mode="determinate", length=440)
        pbar.pack(padx=28, pady=(12, 4))

        detail_var = tk.StringVar(value="0 MB downloaded")
        tk.Label(win, textvariable=detail_var,
                 font=("Consolas", 8), bg=C["bg"], fg=C["accent"]).pack(padx=28, anchor="w")

        def _run():
            tmp_zip = os.path.join(tempfile.gettempdir(), "mox_engine_release.zip")
            try:
                headers = {
                    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) MoxHUB/3.0",
                    "Accept": "*/*"
                }
                req = urllib.request.Request(GITHUB_RELEASE_URL, headers=headers)

                total_size = 0
                try:
                    head_req = urllib.request.Request(GITHUB_RELEASE_URL, headers=headers, method="HEAD")
                    with urllib.request.urlopen(head_req, timeout=8) as hresp:
                        total_size = int(hresp.headers.get("Content-Length", 0))
                except Exception:
                    total_size = 0

                downloaded = 0
                t_start = time.time()
                block_size = 16384

                status_var.set("Downloading package...")

                with urllib.request.urlopen(req, timeout=30) as resp, open(tmp_zip, "wb") as out_f:
                    if total_size == 0:
                        total_size = int(resp.headers.get("Content-Length", 0))
                    total_mb = total_size / (1024 * 1024) if total_size > 0 else 0

                    while True:
                        chunk = resp.read(block_size)
                        if not chunk:
                            break
                        out_f.write(chunk)
                        downloaded += len(chunk)

                        elapsed = max(time.time() - t_start, 0.001)
                        speed_mb = (downloaded / elapsed) / (1024 * 1024)
                        dl_mb = downloaded / (1024 * 1024)
                        pct = (downloaded / total_size * 100) if total_size > 0 else 0

                        pbar["value"] = pct
                        if total_mb > 0:
                            txt = f"{dl_mb:.1f} / {total_mb:.1f} MB  |  {speed_mb:.2f} MB/s  ({pct:.0f}%)"
                        else:
                            txt = f"{dl_mb:.1f} MB downloaded  |  {speed_mb:.2f} MB/s"
                        detail_var.set(txt)
                        win.update_idletasks()

                # Extract ZIP
                status_var.set("Extracting engine files...")
                pbar["value"] = 90
                detail_var.set("Extracting archive...")
                win.update_idletasks()

                if zipfile.is_zipfile(tmp_zip):
                    with zipfile.ZipFile(tmp_zip, "r") as zf:
                        zf.extractall(target_dir)
                else:
                    try:
                        with tarfile.open(tmp_zip, "r:*") as tf:
                            tf.extractall(target_dir)
                    except Exception:
                        shutil.copy(tmp_zip, os.path.join(target_dir, "archive.bin"))

                if os.path.exists(tmp_zip):
                    os.remove(tmp_zip)

                # Rename exe if needed
                for candidate in ("Nova2D.exe", "engine.exe"):
                    old = os.path.join(target_dir, candidate)
                    new = os.path.join(target_dir, "MoxEngine.exe")
                    if os.path.exists(old) and not os.path.exists(new):
                        os.rename(old, new)

                pbar["value"] = 100
                status_var.set("Installation complete!")
                detail_var.set("")
                win.update_idletasks()
                time.sleep(0.3)

                exe_found = self._find_exe_in_dir(target_dir)
                entry = next((x for x in self.engine_versions if x["ver"] == "Mox Engine1"), None)
                if entry:
                    entry["status"] = "installed"
                    entry["path"]   = exe_found or target_dir

                self.active_version = "Mox Engine1"
                self._save_data()
                self.after(0, lambda: [
                    win.destroy(),
                    self._render_version_list(),
                    self._update_status(),
                    messagebox.showinfo(
                        "✅ Mox Engine1 Installed!",
                        f"Mox Engine downloaded & installed successfully!\n\n"
                        f"Location: {target_dir}\n\n"
                        f"Top bar now shows: ● MOX ENGINE READY"
                    )
                ])

            except urllib.error.HTTPError as e:
                if os.path.exists(tmp_zip):
                    try: os.remove(tmp_zip)
                    except Exception: pass
                
                is_404 = (e.code == 404)
                if is_404:
                    err_msg = (
                        f"⚠️ HTTP 404: Engine Release Binary Not Found\n\n"
                        f"The download URL returned 404 Not Found:\n{GITHUB_RELEASE_URL}\n\n"
                        f"Please update GITHUB_RELEASE_URL in HUB/main.py with your actual GitHub release asset link.\n\n"
                        f"Workarounds:\n"
                        f"• Click '📦 Package Local Build' to use your locally compiled binary.\n"
                        f"• Click '📂 Install from Local ZIP' to pick an offline archive."
                    )
                else:
                    err_msg = f"HTTP Error {e.code}: {e.reason}\nURL: {GITHUB_RELEASE_URL}"

                self.after(0, lambda msg=err_msg: [
                    win.destroy(),
                    messagebox.showerror("Download Error", msg)
                ])

            except Exception as e:
                if os.path.exists(tmp_zip):
                    try: os.remove(tmp_zip)
                    except Exception: pass
                self.after(0, lambda err=e: [
                    win.destroy(),
                    messagebox.showerror("Download Error", f"Failed to download/install engine release:\n{err}")
                ])

        threading.Thread(target=_run, daemon=True).start()

    def _show_build_instructions(self):
        """Show a dialog with instructions to build and embed the engine."""
        win = tk.Toplevel(self)
        win.title("Build & Embed Mox Engine")
        win.geometry("520x300")
        win.configure(bg=C["bg"])
        win.resizable(False, False)
        win.grab_set()

        tk.Label(win, text="🔨  Build & Embed Mox Engine",
                 font=("Segoe UI", 13, "bold"), bg=C["bg"], fg=C["fg"]).pack(
                 pady=(20, 8), padx=24, anchor="w")

        steps = [
            ("Step 1", "Make sure MinGW (w64devkit) is installed at C:\\Users\\walaa\\w64devkit"),
            ("Step 2", "Run the following compilation script from the HUB folder:"),
            ("→ File",  "HUB\\compile_engine.bat"),
            ("Result",  "This compiles the engine directly to Engine\\build\\MoxEngine.exe\n"
                        "and runs HUB/embed_engine.py to generate HUB/engine_payload.py."),
        ]
        for lbl, txt in steps:
            row = tk.Frame(win, bg=C["card"], padx=14, pady=8)
            row.pack(fill="x", padx=24, pady=3)
            tk.Label(row, text=lbl, font=("Segoe UI", 8, "bold"),
                     bg=C["card"], fg=C["accent"], width=8, anchor="w").pack(side="left")
            tk.Label(row, text=txt, font=("Consolas", 8),
                     bg=C["card"], fg=C["fg"], anchor="w", wraplength=340,
                     justify="left").pack(side="left", fill="x")

        bot = tk.Frame(win, bg=C["bg"], padx=24, pady=14)
        bot.pack(fill="x", side="bottom")
        self._mk_btn(bot, "Close", win.destroy, bg=C["card"], fg=C["fg"],
                     padx=14, pady=6).pack(side="right")

    # ═══════════════════════════════════════════════════════════════
    # PAGE: Settings
    # ═══════════════════════════════════════════════════════════════
    def _build_page_settings(self):
        page = self.pages["settings"]

        hdr = tk.Frame(page, bg=C["bg"], padx=28, pady=20)
        hdr.pack(fill="x")
        tk.Label(hdr, text="Settings", font=("Segoe UI", 20, "bold"),
                 bg=C["bg"], fg=C["fg"]).pack(side="left")

        container = tk.Frame(page, bg=C["bg"], padx=28)
        container.pack(fill="both", expand=True)

        cfg = [
            ("📁 Hub & Engine Directories", [
                ("Engine Directory",    ENGINE_DIR),
                ("Versions Storage",    VERSIONS_DIR),
                ("Projects Directory",  PROJECTS_DIR),
                ("Database Config",     PROJECTS_DB),
            ]),
            ("⚙️ Environment & Launcher", [
                ("Default Engine Ver",  f"{self.active_version}"),
                ("Graphics API Backend","DirectX 11 (Direct3D 11) / OpenGL (Dual Engine)"),
                ("Target Architecture", "Windows x64 (MinGW/MSVC Hardware Accelerated)"),
                ("Launcher Version",    "Mox HUB v3.0 (Unity-Style Binary Manager)"),
            ]),
        ]

        for section, rows in cfg:
            card = tk.Frame(container, bg=C["card"], padx=18, pady=14)
            card.pack(fill="x", pady=6)
            tk.Label(card, text=section, font=("Segoe UI", 10, "bold"),
                     bg=C["card"], fg=C["accent"]).pack(anchor="w", pady=(0, 8))
            for key, val in rows:
                row = tk.Frame(card, bg=C["card"])
                row.pack(fill="x", pady=3)
                tk.Label(row, text=key, width=22, anchor="w",
                         font=("Segoe UI", 9), bg=C["card"], fg=C["fg2"]).pack(side="left")
                tk.Label(row, text=val, font=("Consolas", 8),
                         bg=C["card"], fg=C["fg"], anchor="w").pack(side="left", fill="x")

        btns = tk.Frame(container, bg=C["bg"], pady=12)
        btns.pack(fill="x")
        for label, path in [("Open Engine Folder", ENGINE_DIR),
                             ("Open Versions Directory", VERSIONS_DIR),
                             ("Open Projects Directory", PROJECTS_DIR)]:
            self._mk_btn(btns, f"📂 {label}",
                         lambda p=path: os.startfile(p) if os.path.exists(p) else None,
                         bg=C["card"], fg=C["fg"], padx=14, pady=6).pack(side="left", padx=(0, 8))

    # ═══════════════════════════════════════════════════════════════
    # Launch Engine & Status
    # ═══════════════════════════════════════════════════════════════
    def launch_engine(self):
        exe_path = self._get_active_exe()
        if not exe_path or not os.path.exists(exe_path):
            messagebox.showwarning("Engine Required",
                f"No pre-compiled binary executable found for {self.active_version}.\n\n"
                "Please go to the 'Installs' tab and download or package a Mox Engine binary.")
            return

        try:
            work_dir = os.path.dirname(exe_path)
            subprocess.Popen([exe_path], cwd=work_dir)
        except Exception as e:
            messagebox.showerror("Launch Error", str(e))

    def _update_status(self):
        exe_path = self._get_active_exe()
        if exe_path and os.path.exists(exe_path):
            self.lbl_status.configure(text=f"● MOX ENGINE READY ({self.active_version})", fg=C["success"])
        else:
            self.lbl_status.configure(text="▲ NO MOX ENGINE INSTALLED", fg=C["warning"])

    # ─── Helpers ───────────────────────────────────────────────────
    def _mk_btn(self, parent, text, command, bg=None, fg=None,
                font=("Segoe UI", 9), padx=10, pady=4, cursor="hand2"):
        bg = bg or C["card"]
        fg = fg or C["fg"]
        btn = tk.Button(parent, text=text, command=command,
                        bg=bg, fg=fg, activebackground=self._lighten(bg),
                        activeforeground=fg, font=font, relief="flat",
                        padx=padx, pady=pady, cursor=cursor, bd=0,
                        highlightthickness=0)
        btn.bind("<Enter>", lambda e: btn.configure(bg=self._lighten(btn.cget("bg"))))
        btn.bind("<Leave>", lambda e: btn.configure(bg=bg))
        return btn

    @staticmethod
    def _lighten(hex_color):
        try:
            hex_color = hex_color.lstrip("#")
            r, g, b = (int(hex_color[i:i+2], 16) for i in (0, 2, 4))
            r, g, b = min(255, r+28), min(255, g+28), min(255, b+28)
            return f"#{r:02x}{g:02x}{b:02x}"
        except Exception:
            return hex_color


# ═══════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    app = MoxHubApp()
    app.mainloop()
