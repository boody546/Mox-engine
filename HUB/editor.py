#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════════
#  Mox Engine 2D — Production-Ready Unity Dark Editor (v4.0)
#  Godot-style 2D Node System, Real-time Asset Import & Python Scripting
# ═══════════════════════════════════════════════════════════════════════════

import os, sys, json, math, time, shutil, subprocess, argparse
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, colorchooser
from datetime import datetime

# Try loading PIL for real image texture rendering in canvas
try:
    from PIL import Image, ImageTk
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

# ─────────────────────────────────────────────────────────────────────────────
# UNITY DARK THEME PALETTE (1:1 replica)
# ─────────────────────────────────────────────────────────────────────────────
U = {
    # Panels
    "bg":           "#282828",
    "panel":        "#383838",
    "panel_dark":   "#3c3c3c",
    "panel_header": "#383838",
    "topbar":       "#383838",
    "sidebar":      "#282828",
    "border":       "#1a1a1a",
    "border_light": "#484848",
    "separator":    "#222222",

    # Text
    "fg":           "#d2d2d2",
    "fg2":          "#8a8a8a",
    "fg3":          "#5a5a5a",
    "fg_dim":       "#6e6e6e",

    # Accents
    "selection":    "#2c5d88",
    "selection_bg": "#1e3d5c",
    "accent_blue":  "#4c7fbe",
    "accent_green": "#4caf50",
    "accent_red":   "#f44336",
    "accent_gold":  "#ffc107",

    # Component labels (Unity Inspector Axis colors)
    "lbl_x":        "#e74c3c",
    "lbl_y":        "#2ecc71",
    "lbl_z":        "#3498db",

    # Input fields
    "input_bg":     "#1e1e1e",
    "input_border": "#2a2a35",
    "border_radius": 8,
    "card_border_radius": 12,
    "input_border_radius": 6,

    # Canvas
    "canvas_bg":    "#202020",
    "grid_line":    "#303030",
    "grid_main":    "#2a2a2a",
    "axis_x":       "#cc3333",
    "axis_y":       "#33aa33",
    "wireframe_green": "#2ecc71",

    # Buttons
    "btn_hover":    "#4a4a4a",
    "btn_active":   "#2c5d88",
    "btn_normal":   "#3c3c3c",

    # Component cards
    "card":         "#212121",
    "card_header":  "#2d2d2d",
    "card_hover":   "#363636",

    # Special
    "play_normal":  "#2d2d2d",
    "play_active":  "#4a93d4",
    "tab_active":   "#383838",
    "tab_inactive": "#2e2e2e",
}

# ─────────────────────────────────────────────────────────────────────────────
# NODE TYPE DEFINITIONS (Including CharacterBody2D, CollisionShape2D, etc.)
# ─────────────────────────────────────────────────────────────────────────────
NODE_TYPES = {
    "Node2D":               {"icon": "◈", "cat": "2D", "col": "#d2d2d2",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,"visible":True,"z_index":0}},
    "Sprite2D":             {"icon": "▣", "cat": "2D", "col": "#4caf50",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,"visible":True,"z_index":0,
                                       "texture":"","modulate":"#ffffff","flip_h":False,"flip_v":False,"width":64,"height":64,
                                       "material":"Default-Material","sorting_layer":"Default"}},
    "AnimatedSprite2D":     {"icon": "▷", "cat": "2D", "col": "#4caf50",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,"visible":True,
                                       "texture":"","columns":4,"rows":4,"fps":12,"frame":0,"playing":True,"loop":True,"modulate":"#ffffff","sorting_layer":"Default"}},
    "Camera2D":             {"icon": "⊙", "cat": "2D", "col": "#ffd700",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"zoom":1.0,"smoothing":True}},
    "CharacterBody2D":      {"icon": "🚶", "cat": "Physics", "col": "#2ecc71",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "speed":200.0,"jump_impulse":-400.0,"gravity":980.0,"velocity_x":0.0,"velocity_y":0.0,
                                       "floor_stop_on_slope":True,"shape":"Box","size_x":40,"size_y":60}},
    "RigidBody2D":          {"icon": "●", "cat": "Physics", "col": "#4c7fbe",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "body_type":"Dynamic","mass":1.0,"gravity_scale":1.0,"friction":0.5,"restitution":0.2,
                                       "physics_material":"Default","shape":"Box","size_x":40,"size_y":40}},
    "StaticBody2D":         {"icon": "▬", "cat": "Physics", "col": "#4c7fbe",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "friction":0.8,"restitution":0.0,"physics_material":"Default","shape":"Box","size_x":100,"size_y":20}},
    "Area2D":               {"icon": "◇", "cat": "Physics", "col": "#8888ff",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "monitoring":True,"shape":"Box","size_x":50,"size_y":50}},
    "CollisionShape2D":     {"icon": "⬡", "cat": "Physics", "col": "#2ecc71",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "shape":"Box","size_x":40,"size_y":40,"radius":20.0,"height":40.0,"disabled":False}},
    "CollisionPolygon2D":   {"icon": "⬠", "cat": "Physics", "col": "#2ecc71",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "polygon_points":"-20,-20, 20,-20, 20,20, -20,20","disabled":False}},
    "TileMap":              {"icon": "⊞", "cat": "Level", "col": "#c8a46e",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"scale_x":1.0,"scale_y":1.0,
                                       "cell_size":32,"columns":20,"rows":20,"tiles":{}}},
    "Light2D":              {"icon": "☀", "cat": "Lighting", "col": "#ffee88",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"color":"#ffffaa","radius":150.0,"energy":1.0}},
    "CanvasModulate":       {"icon": "◐", "cat": "Lighting", "col": "#aaaaff",
                             "props": {"color":"#202030"}},
    "CPUParticles2D":       {"icon": "✦", "cat": "FX",      "col": "#ff8844",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"amount":30,"lifetime":1.5,"speed":100.0,
                                       "color_start":"#ffaa00","color_end":"#ff0000"}},
    "AudioStreamPlayer2D":  {"icon": "🔊", "cat": "Audio",   "col": "#ff66aa",
                             "props": {"position_x":0.0,"position_y":0.0,"stream":"","volume_db":0.0,"pitch_scale":1.0,
                                       "bus":"Master","autoplay":False,"loop":True}},
    "AudioStreamPlayer":    {"icon": "🎵", "cat": "Audio",   "col": "#ff66aa",
                             "props": {"stream":"","volume_db":0.0,"pitch_scale":1.0,"bus":"BGM","autoplay":False,"loop":True}},
    "Label":                {"icon": "T", "cat": "UI",       "col": "#d2d2d2",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"text":"Label","font_size":16,"color":"#ffffff"}},
    "Button":               {"icon": "▭", "cat": "UI",       "col": "#d2d2d2",
                             "props": {"position_x":0.0,"position_y":0.0,"rotation":0.0,"text":"Button","width":100,"height":30,"color":"#4c7fbe"}},
}

TILE_PALETTE = [
    {"id": "grass", "name": "🌱 Grass", "color": "#4caf50"},
    {"id": "dirt",  "name": "🟫 Dirt",  "color": "#795548"},
    {"id": "stone", "name": "🗿 Stone", "color": "#607d8b"},
    {"id": "water", "name": "🌊 Water", "color": "#2196f3"},
    {"id": "brick", "name": "🧱 Brick", "color": "#e91e63"},
    {"id": "wood",  "name": "🪵 Wood",  "color": "#8d6e63"},
]

# ─────────────────────────────────────────────────────────────────────────────
# HELPER WIDGETS
# ─────────────────────────────────────────────────────────────────────────────
def mk_btn(parent, text, cmd, bg=None, fg=None, font=("Segoe UI", 9),
           padx=6, pady=3, relief="flat", active_bg=None, cursor="hand2", **kw):
    bg = bg or U["btn_normal"]
    fg = fg or U["fg"]
    active_bg = active_bg or U["btn_hover"]
    return tk.Button(parent, text=text, command=cmd, bg=bg, fg=fg,
                     activebackground=active_bg, activeforeground=fg,
                     font=font, relief=relief, padx=padx, pady=pady,
                     cursor=cursor, bd=0, **kw)

def mk_separator(parent, orient="h", thickness=1):
    if orient == "h":
        f = tk.Frame(parent, bg=U["border"], height=thickness)
        f.pack(fill="x")
    else:
        f = tk.Frame(parent, bg=U["border"], width=thickness)
        f.pack(fill="y", side="left")
    return f

def mk_input(parent, var=None, width=8):
    e = tk.Entry(parent, textvariable=var, bg=U["input_bg"], fg=U["fg"],
                 insertbackground=U["fg"], bd=1, relief="solid",
                 highlightbackground=U["input_border"],
                 highlightcolor=U["accent_blue"],
                 highlightthickness=1, font=("Segoe UI", 9), width=width)
    return e


# ─────────────────────────────────────────────────────────────────────────────
# MAIN EDITOR CLASS
# ─────────────────────────────────────────────────────────────────────────────
class MoxEditor(tk.Tk):
    def __init__(self, project_path=None):
        super().__init__()

        self.title("Mox Engine 2D — Unity Dark Editor v4.0")
        self.geometry("1440x900")
        self.minsize(1100, 680)
        self.configure(bg=U["border"])

        # ── Project setup ────────────────────────────────────────────
        self.project_path = project_path or os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..", "Projects", "DemoProject"))
        os.makedirs(self.project_path, exist_ok=True)
        os.makedirs(os.path.join(self.project_path, "scenes"), exist_ok=True)
        os.makedirs(os.path.join(self.project_path, "assets"), exist_ok=True)
        os.makedirs(os.path.join(self.project_path, "scripts"), exist_ok=True)

        self.scene_file    = os.path.join(self.project_path, "scenes", "main.json")
        self.colors_db     = os.path.join(self.project_path, ".moxfoldercolors.json")
        self.folder_colors = self._load_json(self.colors_db, {})

        # ── Texture Image Cache ──────────────────────────────────────
        self.texture_cache = {}

        # ── Editor state ─────────────────────────────────────────────
        self.scene_nodes   = []
        self.sel_id        = None
        self.tool          = "move"         # hand | move | rotate | scale | rect | transform | tilemap
        self.grid_snap     = True
        self.grid_size     = 32
        self.zoom          = 1.0
        self.pan_x         = 0
        self.pan_y         = 0
        self.drag_sx       = 0
        self.drag_sy       = 0
        self.drag_orig     = (0.0, 0.0)
        self.show_2d       = True
        self.show_gizmos   = True
        self.engine_proc   = None

        # TileMap Painter state
        self.selected_tile = "grass"
        self.tilemap_mode  = False

        # Animation Timeline state
        self.anim_playing   = False
        self.anim_frame     = 0
        self.anim_max_frames= 30
        self.keyframes      = {}  # {node_id: {frame_num: {prop: val}}}

        # ── Styles ───────────────────────────────────────────────────
        self._setup_ttk_styles()

        # ── Build UI ─────────────────────────────────────────────────
        self._build_menubar()
        self._build_tool_overlay()
        self._build_body()

        # ── Init ─────────────────────────────────────────────────────
        self._load_scene()
        self._refresh_hierarchy()
        self._refresh_asset_browser()
        self._render_viewport()
        self._refresh_inspector()

    # ═══════════════════════════════════════════════════════════════
    # TTK STYLES
    # ═══════════════════════════════════════════════════════════════
    def _setup_ttk_styles(self):
        s = ttk.Style()
        s.theme_use("clam")
        s.configure("Unity.Treeview",
                    background=U["sidebar"], foreground=U["fg"],
                    fieldbackground=U["sidebar"], borderwidth=0,
                    font=("Segoe UI", 9), rowheight=22)
        s.map("Unity.Treeview",
              background=[("selected", U["selection"])],
              foreground=[("selected", "#ffffff")])
        s.configure("Unity.Treeview.Heading",
                    background=U["panel_header"], foreground=U["fg2"],
                    borderwidth=0, font=("Segoe UI", 8, "bold"))
        s.configure("Unity.Vertical.TScrollbar",
                    background=U["panel"], troughcolor=U["border"],
                    arrowcolor=U["fg2"], borderwidth=0)
        s.configure("Unity.TCombobox",
                    fieldbackground=U["input_bg"], background=U["panel"],
                    foreground=U["fg"], borderwidth=1, insertcolor=U["fg"])
        s.map("Unity.TCombobox",
              fieldbackground=[("readonly", U["input_bg"])],
              foreground=[("readonly", U["fg"])])

    # ═══════════════════════════════════════════════════════════════
    # MENU BAR
    # ═══════════════════════════════════════════════════════════════
    def _build_menubar(self):
        menu = tk.Menu(self, bg=U["panel"], fg=U["fg"],
                       activebackground=U["selection"], activeforeground="#fff",
                       relief="flat", bd=0, font=("Segoe UI", 9))
        self.config(menu=menu)

        items = [
            ("File",   [("New Scene",        lambda: self._new_scene()),
                        ("Open Scene...",    lambda: self._open_scene()),
                        ("Save Scene",       lambda: self._save_scene(), "Ctrl+S"),
                        ("---",),
                        ("Build Project",    lambda: self._build_project()),
                        ("---",),
                        ("Exit",             self.quit)]),
            ("Edit",   [("Add Node",         self._show_add_node_menu),
                        ("Duplicate",        self._duplicate_node),
                        ("Delete",           self._delete_node)]),
            ("Assets", [("Import Image/Sound...", lambda: self._import_asset_dialog()),
                        ("New Python Script...",  lambda: self._new_script_dialog()),
                        ("Open in VS Code",       lambda: self._open_in_vscode(self.project_path)),
                        ("Open in Explorer",      lambda: os.startfile(self.project_path))]),
            ("GameObject", [("Node2D",                 lambda: self._add_node("Node2D")),
                            ("Sprite2D",               lambda: self._add_node("Sprite2D")),
                            ("AnimatedSprite2D",       lambda: self._add_node("AnimatedSprite2D")),
                            ("CharacterBody2D",        lambda: self._add_node("CharacterBody2D")),
                            ("CollisionShape2D",       lambda: self._add_node("CollisionShape2D")),
                            ("Camera2D",               lambda: self._add_node("Camera2D")),
                            ("RigidBody2D",            lambda: self._add_node("RigidBody2D")),
                            ("TileMap",                lambda: self._add_node("TileMap")),
                            ("AudioStreamPlayer2D",    lambda: self._add_node("AudioStreamPlayer2D"))]),
            ("Component", [("SpriteRenderer",          lambda: self._add_node("Sprite2D")),
                           ("CollisionShape2D",        lambda: self._add_node("CollisionShape2D")),
                           ("Light2D",                 lambda: self._add_node("Light2D")),
                           ("CPUParticles2D",          lambda: self._add_node("CPUParticles2D")),
                           ("AudioStreamPlayer2D",     lambda: self._add_node("AudioStreamPlayer2D"))]),
            ("Services", [("Project Settings...", self._show_project_settings)]),
            ("Window",  [("Script Editor (Built-in)", self._open_script_editor),
                         ("Open in VS Code",         lambda: self._open_in_vscode(self.project_path)),
                         ("Recompile Engine",        self._recompile_engine)]),
            ("Help",    [("About", lambda: messagebox.showinfo("About",
                                   "Mox Engine 2D\nProduction-Ready Editor v4.0"))]),
        ]

        def make_menu(entries):
            m = tk.Menu(menu, tearoff=0, bg=U["panel"], fg=U["fg"],
                        activebackground=U["selection"], activeforeground="#fff",
                        font=("Segoe UI", 9), relief="flat")
            for entry in entries:
                if entry[0] == "---":
                    m.add_separator()
                else:
                    m.add_command(label=entry[0], command=entry[1])
            return m

        for label, entries in items:
            menu.add_cascade(label=label, menu=make_menu(entries))

        self.bind("<Control-n>", lambda e: self._new_scene())
        self.bind("<Control-o>", lambda e: self._open_scene())
        self.bind("<Control-s>", lambda e: self._save_scene())
        self.bind("<Delete>",    lambda e: self._delete_node())

    # ═══════════════════════════════════════════════════════════════
    # TOOL OVERLAY BAR
    # ═══════════════════════════════════════════════════════════════
    def _build_tool_overlay(self):
        bar = tk.Frame(self, bg=U["panel"], height=40, bd=0)
        bar.pack(side="top", fill="x")
        bar.pack_propagate(False)

        tools_frame = tk.Frame(bar, bg=U["panel"])
        tools_frame.pack(side="left", padx=6, pady=4)

        self._tool_btns = {}
        tools = [
            ("✋", "hand",      "View (Q)"),
            ("⬌",  "move",      "Move (W)"),
            ("↻",  "rotate",    "Rotate (E)"),
            ("⊡",  "scale",     "Scale (R)"),
            ("⬜", "rect",      "Rect Transform (T)"),
            ("⊕",  "transform", "Transform (Y)"),
            ("🎨", "tilemap",   "TileMap Painter"),
        ]
        for icon, mode, tip in tools:
            b = mk_btn(tools_frame, icon, lambda m=mode: self._set_tool(m),
                       bg=U["btn_normal"], fg=U["fg"], padx=8, pady=5,
                       font=("Segoe UI", 11))
            b.pack(side="left", padx=1)
            self._tool_btns[mode] = b

        tk.Frame(bar, bg=U["border"], width=1).pack(side="left", fill="y", padx=6, pady=4)

        # Center: Play / Pause / Step
        center_outer = tk.Frame(bar, bg=U["panel"])
        center_outer.pack(side="left", expand=True)

        play_panel = tk.Frame(center_outer, bg=U["bg"], bd=1, relief="solid")
        play_panel.pack()

        self.btn_play  = mk_btn(play_panel, "▶",  self._on_play,
                                bg=U["bg"], fg=U["fg2"], padx=14, pady=4,
                                font=("Segoe UI", 12, "bold"))
        self.btn_play.pack(side="left")

        tk.Frame(play_panel, bg=U["border"], width=1).pack(side="left", fill="y")

        self.btn_pause = mk_btn(play_panel, "⏸", self._on_pause,
                                bg=U["bg"], fg=U["fg2"], padx=12, pady=4,
                                font=("Segoe UI", 12))
        self.btn_pause.pack(side="left")

        tk.Frame(play_panel, bg=U["border"], width=1).pack(side="left", fill="y")

        self.btn_step  = mk_btn(play_panel, "⏭", self._on_step,
                                bg=U["bg"], fg=U["fg2"], padx=12, pady=4,
                                font=("Segoe UI", 12))
        self.btn_step.pack(side="left")

        right_frame = tk.Frame(bar, bg=U["panel"])
        right_frame.pack(side="right", padx=8, pady=4)

        mk_btn(right_frame, "📝 VS Code", lambda: self._open_in_vscode(self.project_path),
               bg=U["btn_normal"], fg=U["accent_blue"], padx=8, pady=4,
               font=("Segoe UI", 8, "bold")).pack(side="left", padx=3)

        for label in ("Layers ▾", "Layout ▾"):
            b = mk_btn(right_frame, label, lambda: None,
                       bg=U["btn_normal"], fg=U["fg"], padx=10, pady=4,
                       font=("Segoe UI", 9))
            b.pack(side="left", padx=3)

        self._set_tool("move")

    # ═══════════════════════════════════════════════════════════════
    # MAIN BODY
    # ═══════════════════════════════════════════════════════════════
    def _build_body(self):
        outer = tk.PanedWindow(self, orient=tk.VERTICAL,
                               bg=U["border"], sashwidth=3, sashrelief="flat", bd=0)
        outer.pack(fill="both", expand=True)

        work_pane = tk.PanedWindow(outer, orient=tk.HORIZONTAL,
                                   bg=U["border"], sashwidth=3, sashrelief="flat", bd=0)
        outer.add(work_pane, height=620)

        self._build_hierarchy_panel(work_pane)
        self._build_viewport_panel(work_pane)
        self._build_inspector_panel(work_pane)

        self._build_bottom_panel(outer)

    # ═══════════════════════════════════════════════════════════════
    # LEFT: HIERARCHY PANEL
    # ═══════════════════════════════════════════════════════════════
    def _build_hierarchy_panel(self, parent):
        frame = tk.Frame(parent, bg=U["sidebar"], width=240)
        frame.pack_propagate(False)
        parent.add(frame, minsize=180, width=240)

        hdr = tk.Frame(frame, bg=U["panel_header"], height=28, padx=6)
        hdr.pack(fill="x")
        hdr.pack_propagate(False)
        tk.Label(hdr, text="Hierarchy", font=("Segoe UI", 9, "bold"),
                 bg=U["panel_header"], fg=U["fg"]).pack(side="left", pady=4)
        mk_btn(hdr, "+", self._show_add_node_menu,
               bg=U["panel_header"], fg=U["fg2"], padx=6, pady=2,
               font=("Segoe UI", 11)).pack(side="right", pady=3)
        mk_separator(frame)

        search_row = tk.Frame(frame, bg=U["panel"], padx=4, pady=3)
        search_row.pack(fill="x")
        tk.Label(search_row, text="🔍", bg=U["panel"], fg=U["fg3"],
                 font=("Segoe UI", 9)).pack(side="left")
        self._hier_search_var = tk.StringVar()
        e = tk.Entry(search_row, textvariable=self._hier_search_var,
                     bg=U["input_bg"], fg=U["fg"], insertbackground=U["fg"],
                     bd=1, relief="solid", highlightthickness=0,
                     font=("Segoe UI", 9))
        e.pack(side="left", fill="x", expand=True, padx=4)
        self._hier_search_var.trace_add("write", lambda *_: self._refresh_hierarchy())
        mk_separator(frame)

        tree_frame = tk.Frame(frame, bg=U["sidebar"])
        tree_frame.pack(fill="both", expand=True)

        self.hier_tree = ttk.Treeview(tree_frame, style="Unity.Treeview", show="tree")
        sb = ttk.Scrollbar(tree_frame, style="Unity.Vertical.TScrollbar",
                           orient="vertical", command=self.hier_tree.yview)
        self.hier_tree.configure(yscrollcommand=sb.set)
        self.hier_tree.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")

        self.hier_tree.bind("<<TreeviewSelect>>", self._on_hier_select)
        self.hier_tree.bind("<Button-3>", self._show_hier_ctx_menu)
        self.hier_tree.bind("<Double-1>", self._on_hier_double_click)

    # ═══════════════════════════════════════════════════════════════
    # CENTER: VIEWPORT PANEL & TILEMAP DRAWER
    # ═══════════════════════════════════════════════════════════════
    def _build_viewport_panel(self, parent):
        frame = tk.Frame(parent, bg=U["bg"])
        parent.add(frame, minsize=500)

        tab_bar = tk.Frame(frame, bg=U["tab_inactive"], height=26)
        tab_bar.pack(fill="x")
        tab_bar.pack_propagate(False)

        self._scene_tab_active = True
        self.btn_scene_tab = mk_btn(tab_bar, "  ⛰ Scene  ", self._switch_scene_tab,
                                    bg=U["tab_active"], fg=U["fg"], pady=3,
                                    font=("Segoe UI", 9))
        self.btn_scene_tab.pack(side="left")
        self.btn_game_tab  = mk_btn(tab_bar, "  ▷ Game  ", self._switch_game_tab,
                                    bg=U["tab_inactive"], fg=U["fg2"], pady=3,
                                    font=("Segoe UI", 9))
        self.btn_game_tab.pack(side="left")
        mk_separator(frame)

        sub_bar = tk.Frame(frame, bg=U["panel_dark"], height=26)
        sub_bar.pack(fill="x")
        sub_bar.pack_propagate(False)

        self.shaded_var = tk.StringVar(value="Shaded")
        cb = ttk.Combobox(sub_bar, textvariable=self.shaded_var,
                          values=["Shaded", "Wireframe", "Shaded Wireframe"],
                          style="Unity.TCombobox", width=12, state="readonly",
                          font=("Segoe UI", 8))
        cb.pack(side="left", padx=4, pady=3)

        self.var_2d = tk.BooleanVar(value=True)
        self._btn_2d = mk_btn(sub_bar, "2D", self._toggle_2d,
                              bg=U["btn_active"] if self.show_2d else U["btn_normal"],
                              fg="#fff", padx=8, pady=3, font=("Segoe UI", 9, "bold"))
        self._btn_2d.pack(side="left", padx=2)

        self._btn_light = mk_btn(sub_bar, "☀ Lighting", lambda: None,
                                 bg=U["btn_normal"], fg=U["fg2"], padx=6, pady=3,
                                 font=("Segoe UI", 9))
        self._btn_light.pack(side="left", padx=2)

        self._btn_gizmos = mk_btn(sub_bar, "Gizmos ▾", self._toggle_gizmos,
                                  bg=U["btn_active"] if self.show_gizmos else U["btn_normal"],
                                  fg="#fff", padx=8, pady=3, font=("Segoe UI", 9))
        self._btn_gizmos.pack(side="left", padx=2)

        self._btn_tile_drawer = mk_btn(sub_bar, "🎨 Tile Palette", self._toggle_tile_drawer,
                                       bg=U["btn_normal"], fg=U["fg2"], padx=8, pady=3,
                                       font=("Segoe UI", 8, "bold"))
        self._btn_tile_drawer.pack(side="left", padx=6)

        self.lbl_zoom = tk.Label(sub_bar, text="100%", bg=U["panel_dark"],
                                 fg=U["fg3"], font=("Consolas", 8))
        self.lbl_zoom.pack(side="right", padx=6)
        self.lbl_cursor = tk.Label(sub_bar, text="(0.0, 0.0)", bg=U["panel_dark"],
                                   fg=U["fg3"], font=("Consolas", 8))
        self.lbl_cursor.pack(side="right", padx=6)

        self.tile_drawer = tk.Frame(frame, bg=U["panel"], height=36)
        self._build_tile_drawer_content()

        self.canvas = tk.Canvas(frame, bg=U["canvas_bg"], highlightthickness=0, bd=0)
        self.canvas.pack(fill="both", expand=True)

        self.canvas.bind("<ButtonPress-1>",   self._cv_click)
        self.canvas.bind("<B1-Motion>",       self._cv_drag)
        self.canvas.bind("<ButtonRelease-1>", self._cv_release)
        self.canvas.bind("<ButtonPress-2>",   self._pan_start)
        self.canvas.bind("<B2-Motion>",       self._pan_drag)
        self.canvas.bind("<ButtonPress-3>",   self._pan_start)
        self.canvas.bind("<B3-Motion>",       self._pan_drag)
        self.canvas.bind("<MouseWheel>",      self._cv_scroll)
        self.canvas.bind("<Motion>",          self._cv_motion)

    def _build_tile_drawer_content(self):
        tk.Label(self.tile_drawer, text="Tile Palette:", font=("Segoe UI", 8, "bold"),
                 bg=U["panel"], fg=U["fg"]).pack(side="left", padx=8)

        self._tile_btns = {}
        for t in TILE_PALETTE:
            b = mk_btn(self.tile_drawer, t["name"],
                       lambda tid=t["id"]: self._select_tile(tid),
                       bg=t["color"] if self.selected_tile == t["id"] else U["btn_normal"],
                       fg="#ffffff", padx=8, pady=2, font=("Segoe UI", 8))
            b.pack(side="left", padx=2, pady=4)
            self._tile_btns[t["id"]] = b

    def _toggle_tile_drawer(self):
        if self.tile_drawer.winfo_ismapped():
            self.tile_drawer.pack_forget()
            self._btn_tile_drawer.configure(bg=U["btn_normal"], fg=U["fg2"])
            self.tilemap_mode = False
        else:
            self.tile_drawer.pack(fill="x", before=self.canvas)
            self._btn_tile_drawer.configure(bg=U["btn_active"], fg="#ffffff")
            self.tilemap_mode = True

    def _select_tile(self, tile_id):
        self.selected_tile = tile_id
        for tid, b in self._tile_btns.items():
            t_col = next((t["color"] for t in TILE_PALETTE if t["id"] == tid), U["btn_normal"])
            b.configure(bg=t_col if tid == tile_id else U["btn_normal"])

    # ═══════════════════════════════════════════════════════════════
    # RIGHT: INSPECTOR PANEL
    # ═══════════════════════════════════════════════════════════════
    def _build_inspector_panel(self, parent):
        frame = tk.Frame(parent, bg=U["sidebar"], width=290)
        frame.pack_propagate(False)
        parent.add(frame, minsize=240, width=290)

        hdr = tk.Frame(frame, bg=U["panel_header"], height=28, padx=6)
        hdr.pack(fill="x")
        hdr.pack_propagate(False)
        tk.Label(hdr, text="Inspector", font=("Segoe UI", 9, "bold"),
                 bg=U["panel_header"], fg=U["fg"]).pack(side="left", pady=4)
        mk_btn(hdr, "⋮", lambda: None, bg=U["panel_header"], fg=U["fg2"],
               padx=6, pady=2, font=("Segoe UI", 12)).pack(side="right", pady=3)
        mk_separator(frame)

        self.insp_canvas = tk.Canvas(frame, bg=U["sidebar"], highlightthickness=0, bd=0)
        insp_sb = ttk.Scrollbar(frame, style="Unity.Vertical.TScrollbar",
                                orient="vertical", command=self.insp_canvas.yview)
        self.insp_inner = tk.Frame(self.insp_canvas, bg=U["sidebar"])
        self.insp_inner.bind("<Configure>", lambda e: self.insp_canvas.configure(
            scrollregion=self.insp_canvas.bbox("all")))
        self.insp_canvas.create_window((0, 0), window=self.insp_inner, anchor="nw")
        self.insp_canvas.configure(yscrollcommand=insp_sb.set)

        self.insp_canvas.pack(side="left", fill="both", expand=True)
        insp_sb.pack(side="right", fill="y")

    # ═══════════════════════════════════════════════════════════════
    # BOTTOM: PROJECT / CONSOLE / ANIMATION TABS
    # ═══════════════════════════════════════════════════════════════
    def _build_bottom_panel(self, parent):
        self.bottom_frame = tk.Frame(parent, bg=U["sidebar"])
        parent.add(self.bottom_frame, height=220)

        tab_bar = tk.Frame(self.bottom_frame, bg=U["tab_inactive"], height=26)
        tab_bar.pack(fill="x")
        tab_bar.pack_propagate(False)

        self._bottom_tab = "project"
        self._bottom_tab_btns = {}
        for tab_name, label in [("project","  ▤ Project  "),("console","  ≡ Console  "),("animation","  🎬 Animation  ")]:
            b = mk_btn(tab_bar, label,
                       lambda t=tab_name: self._switch_bottom_tab(t),
                       bg=U["tab_active"] if tab_name == "project" else U["tab_inactive"],
                       fg=U["fg"] if tab_name == "project" else U["fg2"],
                       pady=3, font=("Segoe UI", 9))
            b.pack(side="left")
            self._bottom_tab_btns[tab_name] = b
        mk_separator(self.bottom_frame)

        self.bottom_content = tk.Frame(self.bottom_frame, bg=U["sidebar"])
        self.bottom_content.pack(fill="both", expand=True)

        self._build_project_tab_content()

    def _switch_bottom_tab(self, tab):
        self._bottom_tab = tab
        for t, b in self._bottom_tab_btns.items():
            b.configure(bg=U["tab_active"] if t==tab else U["tab_inactive"],
                        fg=U["fg"] if t==tab else U["fg2"])

        for w in self.bottom_content.winfo_children():
            w.destroy()

        if tab == "project":
            self._build_project_tab_content()
        elif tab == "console":
            self._build_console_tab_content()
        elif tab == "animation":
            self._build_animation_tab_content()

    def _build_project_tab_content(self):
        sub = tk.Frame(self.bottom_content, bg=U["panel"], height=26)
        sub.pack(fill="x")
        sub.pack_propagate(False)
        mk_btn(sub, "+ Import Asset...", self._import_asset_dialog, bg=U["panel"], fg=U["fg2"],
               padx=8, pady=3, font=("Segoe UI", 9)).pack(side="left", padx=4)
        mk_btn(sub, "+ New Script", lambda: self._new_script_dialog(), bg=U["panel"], fg=U["accent_blue"],
               padx=8, pady=3, font=("Segoe UI", 9, "bold")).pack(side="left", padx=2)
        tk.Frame(sub, bg=U["border"], width=1).pack(side="left", fill="y", pady=3)

        search_row = tk.Frame(sub, bg=U["panel"])
        search_row.pack(side="right", padx=6, pady=3)
        tk.Label(search_row, text="🔍", bg=U["panel"], fg=U["fg3"],
                 font=("Segoe UI", 9)).pack(side="left")
        self._asset_search_var = tk.StringVar()
        tk.Entry(search_row, textvariable=self._asset_search_var,
                 bg=U["input_bg"], fg=U["fg"], insertbackground=U["fg"],
                 bd=1, relief="solid", highlightthickness=0,
                 font=("Segoe UI", 9), width=18).pack(side="left")
        mk_separator(self.bottom_content)

        content = tk.PanedWindow(self.bottom_content, orient=tk.HORIZONTAL,
                                 bg=U["border"], sashwidth=3, bd=0)
        content.pack(fill="both", expand=True)

        tree_frame = tk.Frame(content, bg=U["sidebar"], width=220)
        tree_frame.pack_propagate(False)
        content.add(tree_frame, minsize=160, width=220)

        self.folder_tree = ttk.Treeview(tree_frame, style="Unity.Treeview", show="tree")
        fsb = ttk.Scrollbar(tree_frame, style="Unity.Vertical.TScrollbar",
                            orient="vertical", command=self.folder_tree.yview)
        self.folder_tree.configure(yscrollcommand=fsb.set)
        self.folder_tree.pack(side="left", fill="both", expand=True)
        fsb.pack(side="right", fill="y")
        self.folder_tree.bind("<ButtonRelease-1>", self._on_folder_select)

        self.icon_frame_outer = tk.Frame(content, bg=U["bg"])
        content.add(self.icon_frame_outer, minsize=300)

        self.icon_canvas = tk.Canvas(self.icon_frame_outer, bg=U["bg"],
                                     highlightthickness=0, bd=0)
        icon_sb = ttk.Scrollbar(self.icon_frame_outer, style="Unity.Vertical.TScrollbar",
                                orient="vertical", command=self.icon_canvas.yview)
        self.icon_inner = tk.Frame(self.icon_canvas, bg=U["bg"])
        self.icon_inner.bind("<Configure>", lambda e: self.icon_canvas.configure(
            scrollregion=self.icon_canvas.bbox("all")))
        self.icon_canvas.create_window((0, 0), window=self.icon_inner, anchor="nw")
        self.icon_canvas.configure(yscrollcommand=icon_sb.set)
        self.icon_canvas.pack(side="left", fill="both", expand=True)
        icon_sb.pack(side="right", fill="y")

        self.folder_tree.bind("<Button-3>", self._show_asset_ctx_menu)
        self.icon_inner.bind("<Button-3>", self._show_asset_ctx_menu)
        self._current_folder = self.project_path
        self._refresh_asset_browser()

    def _build_console_tab_content(self):
        console_frame = tk.Frame(self.bottom_content, bg="#1a1a1a")
        console_frame.pack(fill="both", expand=True, padx=4, pady=4)
        txt = tk.Text(console_frame, bg="#141414", fg="#a0e0a0", insertbackground="#fff",
                      font=("Consolas", 9), bd=0, padx=8, pady=8)
        txt.pack(fill="both", expand=True)
        txt.insert("end", f"[{datetime.now().strftime('%H:%M:%S')}] Mox Engine Production Runtime initialized.\n")
        txt.insert("end", f"[{datetime.now().strftime('%H:%M:%S')}] Active Project: {self.project_path}\n")
        txt.insert("end", f"[{datetime.now().strftime('%H:%M:%S')}] Scene Export Location: scenes/main.json\n")
        txt.insert("end", f"[{datetime.now().strftime('%H:%M:%S')}] Ready.\n")
        txt.configure(state="disabled")

    def _build_animation_tab_content(self):
        anim_frame = tk.Frame(self.bottom_content, bg=U["sidebar"])
        anim_frame.pack(fill="both", expand=True)

        toolbar = tk.Frame(anim_frame, bg=U["panel"], height=32, padx=8)
        toolbar.pack(fill="x")
        toolbar.pack_propagate(False)

        self.btn_anim_play = mk_btn(toolbar, "▶ Play Anim", self._toggle_anim_play,
                                    bg=U["btn_normal"], fg=U["fg"], padx=8, pady=2,
                                    font=("Segoe UI", 8, "bold"))
        self.btn_anim_play.pack(side="left", padx=4)

        mk_btn(toolbar, "🔑 + Keyframe", self._add_keyframe,
               bg=U["accent_blue"], fg="#fff", padx=8, pady=2,
               font=("Segoe UI", 8, "bold")).pack(side="left", padx=4)

        self.lbl_anim_frame = tk.Label(toolbar, text=f"Frame: {self.anim_frame} / {self.anim_max_frames}",
                                       bg=U["panel"], fg=U["fg2"], font=("Consolas", 9))
        self.lbl_anim_frame.pack(side="left", padx=12)

        slider_frame = tk.Frame(anim_frame, bg=U["panel_dark"], height=36, padx=12, pady=4)
        slider_frame.pack(fill="x")

        self.anim_slider = tk.Scale(slider_frame, from_=0, to=self.anim_max_frames,
                                    orient="horizontal", bg=U["panel_dark"], fg=U["fg"],
                                    highlightthickness=0, troughcolor=U["input_bg"],
                                    command=self._on_anim_slider)
        self.anim_slider.set(self.anim_frame)
        self.anim_slider.pack(fill="x", expand=True)

        tracks_frame = tk.Frame(anim_frame, bg=U["bg"])
        tracks_frame.pack(fill="both", expand=True, padx=4, pady=4)

        self.anim_tracks_list = tk.Text(tracks_frame, bg=U["input_bg"], fg=U["fg"],
                                        font=("Consolas", 8), bd=0, padx=8, pady=6)
        self.anim_tracks_list.pack(fill="both", expand=True)
        self._refresh_anim_tracks_view()

    def _toggle_anim_play(self):
        self.anim_playing = not self.anim_playing
        self.btn_anim_play.configure(bg=U["play_active"] if self.anim_playing else U["btn_normal"])
        if self.anim_playing:
            self._anim_step()

    def _anim_step(self):
        if not self.anim_playing: return
        self.anim_frame = (self.anim_frame + 1) % (self.anim_max_frames + 1)
        if hasattr(self, 'anim_slider'):
            self.anim_slider.set(self.anim_frame)
        self._apply_anim_keyframe(self.anim_frame)
        self.after(60, self._anim_step)

    def _on_anim_slider(self, val):
        self.anim_frame = int(val)
        if hasattr(self, 'lbl_anim_frame'):
            self.lbl_anim_frame.configure(text=f"Frame: {self.anim_frame} / {self.anim_max_frames}")
        self._apply_anim_keyframe(self.anim_frame)

    def _add_keyframe(self):
        node = self._sel_node()
        if not node:
            messagebox.showwarning("Animation", "Select a node first to add keyframe.")
            return
        nid = node["id"]
        self.keyframes.setdefault(nid, {})
        self.keyframes[nid][self.anim_frame] = {
            "position_x": node["props"].get("position_x", 0),
            "position_y": node["props"].get("position_y", 0),
            "rotation": node["props"].get("rotation", 0),
            "frame": node["props"].get("frame", 0),
            "modulate": node["props"].get("modulate", "#ffffff")
        }
        self._refresh_anim_tracks_view()

    def _apply_anim_keyframe(self, frame):
        node = self._sel_node()
        if not node: return
        nid = node["id"]
        if nid in self.keyframes and frame in self.keyframes[nid]:
            kf = self.keyframes[nid][frame]
            for k, v in kf.items():
                node["props"][k] = v
            self._refresh_inspector()
            self._render_viewport()

    def _refresh_anim_tracks_view(self):
        if not hasattr(self, 'anim_tracks_list'): return
        self.anim_tracks_list.delete("1.0", "end")
        node = self._sel_node()
        if not node:
            self.anim_tracks_list.insert("end", "No node selected for keyframing.\n")
            return
        nid = node["id"]
        self.anim_tracks_list.insert("end", f"Tracks for: {node['name']} ({node['type']})\n")
        self.anim_tracks_list.insert("end", "─" * 45 + "\n")
        if nid in self.keyframes and self.keyframes[nid]:
            for f in sorted(self.keyframes[nid].keys()):
                kf = self.keyframes[nid][f]
                self.anim_tracks_list.insert("end", f"  • Frame {f:02d}: pos=({kf.get('position_x')}, {kf.get('position_y')}) rot={kf.get('rotation')} mod={kf.get('modulate')}\n")
        else:
            self.anim_tracks_list.insert("end", "  (No keyframes recorded. Click '+ Keyframe' to record state)\n")

    # ═══════════════════════════════════════════════════════════════
    # HIERARCHY REFRESH
    # ═══════════════════════════════════════════════════════════════
    def _refresh_hierarchy(self):
        search = self._hier_search_var.get().lower()
        for item in self.hier_tree.get_children():
            self.hier_tree.delete(item)

        for node in self.scene_nodes:
            ntype = node["type"]
            meta  = NODE_TYPES.get(ntype, {})
            icon  = meta.get("icon", "◈")
            col   = meta.get("col", U["fg"])
            label = f"{icon}  {node['name']}  [{ntype}]"

            if search and search not in node["name"].lower() and search not in ntype.lower():
                continue

            parent_id = node.get("parent") or ""
            try:
                self.hier_tree.insert(parent_id, "end", iid=node["id"],
                                      text=label, open=True, tags=(col,))
            except tk.TclError:
                self.hier_tree.insert("", "end", iid=node["id"],
                                      text=label, open=True, tags=(col,))
            self.hier_tree.tag_configure(col, foreground=col)

        if self.sel_id and self.hier_tree.exists(self.sel_id):
            self.hier_tree.selection_set(self.sel_id)
            self.hier_tree.see(self.sel_id)

    def _on_hier_select(self, event):
        sel = self.hier_tree.selection()
        if sel:
            self.sel_id = sel[0]
            self._refresh_inspector()
            self._render_viewport()
            if hasattr(self, 'anim_tracks_list'):
                self._refresh_anim_tracks_view()

    def _on_hier_double_click(self, event):
        node = self._sel_node()
        if node:
            self.pan_x = -node["props"].get("position_x", 0) * self.zoom
            self.pan_y = -node["props"].get("position_y", 0) * self.zoom
            self._render_viewport()

    def _show_hier_ctx_menu(self, event):
        item = self.hier_tree.identify_row(event.y)
        if item:
            self.hier_tree.selection_set(item)
            self.sel_id = item
        m = tk.Menu(self, tearoff=0, bg=U["panel"], fg=U["fg"],
                    activebackground=U["selection"], activeforeground="#fff",
                    font=("Segoe UI", 9))
        m.add_command(label="Add Node...",       command=self._show_add_node_menu)
        m.add_command(label="Duplicate",         command=self._duplicate_node)
        m.add_command(label="Attach Script",     command=lambda: self._new_script_dialog(self._sel_node()))
        m.add_separator()
        m.add_command(label="Delete",            command=self._delete_node)
        m.post(event.x_root, event.y_root)

    # ═══════════════════════════════════════════════════════════════
    # INSPECTOR REFRESH — Ultra-Polished Modern Unity Cards
    # ═══════════════════════════════════════════════════════════════
    def _refresh_inspector(self):
        for w in self.insp_inner.winfo_children():
            w.destroy()

        node = self._sel_node()
        if not node:
            tk.Label(self.insp_inner, text="\nNo object selected",
                     bg=U["sidebar"], fg=U["fg3"],
                     font=("Segoe UI", 9)).pack(pady=20)
            return

        top = tk.Frame(self.insp_inner, bg=U["panel"], padx=8, pady=6)
        top.pack(fill="x", pady=(0, 2))

        left_top = tk.Frame(top, bg=U["panel"])
        left_top.pack(fill="x")

        var_vis = tk.BooleanVar(value=node["props"].get("visible", True))
        tk.Checkbutton(left_top, variable=var_vis, bg=U["panel"],
                       activebackground=U["panel"], selectcolor=U["input_bg"],
                       command=lambda: self._prop_set(node, "visible", var_vis.get())).pack(side="left")

        name_var = tk.StringVar(value=node["name"])
        name_entry = mk_input(left_top, name_var, width=18)
        name_entry.pack(side="left", padx=(4, 8))
        name_entry.bind("<FocusOut>", lambda e: self._on_name_change(node, name_var.get()))

        tag_row = tk.Frame(top, bg=U["panel"])
        tag_row.pack(fill="x", pady=(4, 0))

        tk.Label(tag_row, text="Tag", bg=U["panel"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=5, anchor="w").pack(side="left")
        tag_var = tk.StringVar(value=node.get("tag", "Untagged"))
        cb_tag = ttk.Combobox(tag_row, textvariable=tag_var, style="Unity.TCombobox",
                              values=["Untagged", "Player", "Enemy", "MainCamera", "Environment"],
                              width=10, state="readonly", font=("Segoe UI", 8))
        cb_tag.pack(side="left", padx=(0, 8))
        cb_tag.bind("<<ComboboxSelected>>", lambda e: node.update({"tag": tag_var.get()}))

        tk.Label(tag_row, text="Layer", bg=U["panel"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=5, anchor="w").pack(side="left")
        layer_var = tk.StringVar(value=node.get("layer", "Default"))
        cb_layer = ttk.Combobox(tag_row, textvariable=layer_var, style="Unity.TCombobox",
                                values=["Default", "UI", "Background", "Player", "Enemy"],
                                width=10, state="readonly", font=("Segoe UI", 8))
        cb_layer.pack(side="left")
        cb_layer.bind("<<ComboboxSelected>>", lambda e: node.update({"layer": layer_var.get()}))

        # ── Transform Card ─────────────────────────────────────────
        self._mk_component_card("Transform", node, [
            ("Position", [("X","position_x",U["lbl_x"]),("Y","position_y",U["lbl_y"])]),
            ("Rotation", [("Z","rotation",U["lbl_z"])]),
            ("Scale",    [("X","scale_x",U["lbl_x"]),("Y","scale_y",U["lbl_y"])]),
        ])

        # ── Sprite Renderer Card ───────────────────────────────────
        if node["type"] in ("Sprite2D", "AnimatedSprite2D"):
            self._mk_sprite_renderer_card(node)

        # ── Physics Cards ──────────────────────────────────────────
        if node["type"] in ("RigidBody2D", "StaticBody2D", "Area2D", "CharacterBody2D"):
            self._mk_physics_card(node)

        # ── Collision Shape / Polygon Cards ────────────────────────
        if node["type"] in ("CollisionShape2D", "CollisionPolygon2D"):
            self._mk_collision_card(node)

        # ── Audio Stream Card ──────────────────────────────────────
        if node["type"] in ("AudioStreamPlayer2D", "AudioStreamPlayer"):
            self._mk_audio_card(node)

        # ── Light Card ─────────────────────────────────────────────
        if node["type"] == "Light2D":
            self._mk_light_card(node)

        # ── Generic Props ──────────────────────────────────────────
        leftover = {k: v for k, v in node["props"].items()
                    if k not in ("position_x","position_y","rotation","scale_x","scale_y",
                                 "visible","texture","modulate","flip_h","flip_v","color",
                                 "radius","energy","mass","gravity_scale","shape","size_x","size_y",
                                 "friction","restitution","physics_material","monitoring","width","height","z_index",
                                 "stream","volume_db","pitch_scale","bus","autoplay","loop","material","sorting_layer",
                                 "speed","jump_impulse","gravity","velocity_x","velocity_y","floor_stop_on_slope",
                                 "columns","rows","fps","frame","playing","polygon_points","disabled")}
        if leftover:
            self._mk_generic_card(node["type"] + " Properties", node, leftover)

        # ── Script Field ───────────────────────────────────────────
        script_card = self._mk_card_frame("Python Script")
        s_path = node["props"].get("script", "")
        if s_path:
            s_row = tk.Frame(script_card, bg=U["card"])
            s_row.pack(fill="x", pady=2)
            tk.Label(s_row, text=os.path.basename(s_path), bg=U["card"], fg=U["accent_blue"],
                     font=("Consolas", 9, "bold")).pack(side="left")
            mk_btn(s_row, "📝 VS Code", lambda: self._open_in_vscode(s_path),
                   bg=U["input_bg"], fg=U["fg"], padx=6, pady=2, font=("Segoe UI", 8)).pack(side="right")
        else:
            mk_btn(script_card, "+ Attach / Create Script", lambda: self._new_script_dialog(node),
                   bg=U["input_bg"], fg=U["fg2"], padx=8, pady=3, font=("Segoe UI", 8)).pack(fill="x")

        # ── Add Component Button ───────────────────────────────────
        gap = tk.Frame(self.insp_inner, bg=U["sidebar"], height=10)
        gap.pack(fill="x")
        btn_add_comp = mk_btn(self.insp_inner, "Add Component",
                               self._open_add_component_popup,
                               bg=U["panel"], fg="#ffffff", padx=0, pady=7,
                               font=("Segoe UI", 9, "bold"), width=40)
        btn_add_comp.pack(fill="x", padx=12, pady=6)

    def _mk_card_frame(self, title, collapsible=True):
        outer = tk.Frame(self.insp_inner, bg=U["sidebar"], pady=2)
        outer.pack(fill="x")

        header = tk.Frame(outer, bg=U["card_header"], padx=6, pady=4, bd=1, relief="solid",
                          highlightbackground=U["input_border"], highlightthickness=1)
        header.pack(fill="x")

        arrow_var = tk.StringVar(value="▼")
        arrow_btn = tk.Label(header, textvariable=arrow_var,
                             bg=U["card_header"], fg=U["fg2"],
                             font=("Segoe UI", 8), cursor="hand2")
        arrow_btn.pack(side="left")

        tk.Label(header, text=f"  {title}", font=("Segoe UI", 9, "bold"),
                 bg=U["card_header"], fg=U["fg"]).pack(side="left")

        options_btn = tk.Label(header, text="⋮", font=("Segoe UI", 11),
                               bg=U["card_header"], fg=U["fg2"], cursor="hand2")
        options_btn.pack(side="right")

        body = tk.Frame(outer, bg=U["card"], padx=10, pady=6)
        body.pack(fill="x")

        def toggle():
            if body.winfo_ismapped():
                body.pack_forget()
                arrow_var.set("►")
            else:
                body.pack(fill="x")
                arrow_var.set("▼")

        arrow_btn.bind("<Button-1>", lambda e: toggle())
        header.bind("<Button-1>", lambda e: toggle())
        return body

    def _mk_component_card(self, title, node, field_groups):
        body = self._mk_card_frame(title)
        for group_label, fields in field_groups:
            row = tk.Frame(body, bg=U["card"])
            row.pack(fill="x", pady=2)
            tk.Label(row, text=group_label, bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=9, anchor="w").pack(side="left")
            for axis_label, prop_key, axis_col in fields:
                tk.Label(row, text=axis_label, bg=axis_col, fg="#ffffff",
                         font=("Segoe UI", 8, "bold"), width=2,
                         padx=3, pady=2).pack(side="left")
                v = tk.StringVar(value=str(round(node["props"].get(prop_key, 0), 3)))
                e = mk_input(row, v, width=7)
                e.pack(side="left", padx=(0, 4))
                e.bind("<FocusOut>", lambda ev, k=prop_key, var=v: self._on_float_change(node, k, var.get()))

    def _mk_sprite_renderer_card(self, node):
        body = self._mk_card_frame(f"{node['type']} (Sprite Renderer)")

        # Texture picker slot
        row1 = tk.Frame(body, bg=U["card"])
        row1.pack(fill="x", pady=2)
        tk.Label(row1, text="Texture", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        spr_path = node["props"].get("texture","") or "None (Sprite)"
        mk_btn(row1, os.path.basename(spr_path) or "Select Texture",
               lambda: self._pick_texture(node), bg=U["input_bg"], fg=U["fg"],
               padx=6, pady=2, font=("Segoe UI", 8)).pack(side="left", fill="x", expand=True)

        if node["type"] == "AnimatedSprite2D":
            # SpriteSheet slicing tools
            row_slice = tk.Frame(body, bg=U["card"])
            row_slice.pack(fill="x", pady=2)
            tk.Label(row_slice, text="Grid (Cols/Rows)", bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            v_cols = tk.StringVar(value=str(node["props"].get("columns", 4)))
            e_cols = mk_input(row_slice, v_cols, width=4)
            e_cols.pack(side="left", padx=2)
            e_cols.bind("<FocusOut>", lambda e: self._prop_set(node, "columns", int(v_cols.get() or 1)))

            v_rows = tk.StringVar(value=str(node["props"].get("rows", 4)))
            e_rows = mk_input(row_slice, v_rows, width=4)
            e_rows.pack(side="left", padx=2)
            e_rows.bind("<FocusOut>", lambda e: self._prop_set(node, "rows", int(v_rows.get() or 1)))

            # FPS and Frame Scrub
            row_fps = tk.Frame(body, bg=U["card"])
            row_fps.pack(fill="x", pady=2)
            tk.Label(row_fps, text="FPS / Frame", bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            v_fps = tk.StringVar(value=str(node["props"].get("fps", 12)))
            e_fps = mk_input(row_fps, v_fps, width=5)
            e_fps.pack(side="left", padx=2)
            e_fps.bind("<FocusOut>", lambda e: self._prop_set(node, "fps", int(v_fps.get() or 12)))

            v_frame = tk.StringVar(value=str(node["props"].get("frame", 0)))
            e_frame = mk_input(row_fps, v_frame, width=5)
            e_frame.pack(side="left", padx=2)
            e_frame.bind("<FocusOut>", lambda e: self._prop_set(node, "frame", int(v_frame.get() or 0)))

        # Color swatch
        row2 = tk.Frame(body, bg=U["card"])
        row2.pack(fill="x", pady=2)
        tk.Label(row2, text="Modulate Color", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        col = node["props"].get("modulate","#ffffff")
        color_swatch = tk.Label(row2, text="         ", bg=col, relief="solid", bd=1, cursor="hand2")
        color_swatch.pack(side="left", padx=(0, 4))
        color_swatch.bind("<Button-1>", lambda e: self._pick_modulate(node, color_swatch))

        # Material Dropdown
        row_mat = tk.Frame(body, bg=U["card"])
        row_mat.pack(fill="x", pady=2)
        tk.Label(row_mat, text="Material", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        mat_var = tk.StringVar(value=node["props"].get("material", "Default-Material"))
        cb_mat = ttk.Combobox(row_mat, textvariable=mat_var, style="Unity.TCombobox",
                              values=["Default-Material", "Sprites-Default", "Pixel-Lit", "Unlit-Transparent"],
                              state="readonly", font=("Segoe UI", 8))
        cb_mat.pack(side="left", fill="x", expand=True)
        cb_mat.bind("<<ComboboxSelected>>", lambda e: self._prop_set(node, "material", mat_var.get()))

        # Flip X / Y
        row3 = tk.Frame(body, bg=U["card"])
        row3.pack(fill="x", pady=2)
        tk.Label(row3, text="Flip", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        for axis in ("X", "Y"):
            key = f"flip_{axis.lower()}"
            var = tk.BooleanVar(value=node["props"].get(key, False))
            tk.Checkbutton(row3, text=axis, variable=var, bg=U["card"],
                           fg=U["fg"], activebackground=U["card"],
                           selectcolor=U["input_bg"],
                           command=lambda k=key, v=var: self._prop_set(node, k, v.get()),
                           font=("Segoe UI", 8)).pack(side="left", padx=6)

    def _mk_physics_card(self, node):
        body = self._mk_card_frame(f"{node['type']} (Physics 2D)")

        if node["type"] == "CharacterBody2D":
            for key in ("speed", "jump_impulse", "gravity", "velocity_x", "velocity_y"):
                row = tk.Frame(body, bg=U["card"])
                row.pack(fill="x", pady=2)
                tk.Label(row, text=key.replace("_"," ").title(), bg=U["card"], fg=U["fg2"],
                         font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
                v = tk.StringVar(value=str(node["props"].get(key, 0.0)))
                e = mk_input(row, v, width=10)
                e.pack(side="left")
                e.bind("<FocusOut>", lambda ev, k=key, var=v: self._on_float_change(node, k, var.get()))
        else:
            row_mat = tk.Frame(body, bg=U["card"])
            row_mat.pack(fill="x", pady=2)
            tk.Label(row_mat, text="Physics Mat", bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            pmat_var = tk.StringVar(value=node["props"].get("physics_material", "Default"))
            cb_pmat = ttk.Combobox(row_mat, textvariable=pmat_var, style="Unity.TCombobox",
                                   values=["Default", "Bouncy", "Frictionless", "Ice", "Rubber"],
                                   state="readonly", font=("Segoe UI", 8))
            cb_pmat.pack(side="left", fill="x", expand=True)

            def _on_pmat_select(e):
                mat = pmat_var.get()
                node["props"]["physics_material"] = mat
                if mat == "Bouncy": node["props"]["restitution"] = 0.9; node["props"]["friction"] = 0.2
                elif mat == "Frictionless": node["props"]["friction"] = 0.0
                elif mat == "Ice": node["props"]["friction"] = 0.05
                elif mat == "Rubber": node["props"]["restitution"] = 0.7; node["props"]["friction"] = 0.8
                self._refresh_inspector()
            cb_pmat.bind("<<ComboboxSelected>>", _on_pmat_select)

            for key in ("mass", "gravity_scale", "friction", "restitution", "size_x", "size_y"):
                if key in node["props"]:
                    row = tk.Frame(body, bg=U["card"])
                    row.pack(fill="x", pady=2)
                    tk.Label(row, text=key.replace("_"," ").title(), bg=U["card"], fg=U["fg2"],
                             font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
                    v = tk.StringVar(value=str(node["props"][key]))
                    e = mk_input(row, v, width=10)
                    e.pack(side="left")
                    e.bind("<FocusOut>", lambda ev, k=key, var=v: self._on_float_change(node, k, var.get()))

    def _mk_collision_card(self, node):
        body = self._mk_card_frame(f"{node['type']} (Collider)")

        if node["type"] == "CollisionShape2D":
            row_shape = tk.Frame(body, bg=U["card"])
            row_shape.pack(fill="x", pady=2)
            tk.Label(row_shape, text="Shape", bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            shape_var = tk.StringVar(value=node["props"].get("shape", "Box"))
            cb_shape = ttk.Combobox(row_shape, textvariable=shape_var, style="Unity.TCombobox",
                                    values=["Box", "Circle", "Capsule"], state="readonly", font=("Segoe UI", 8))
            cb_shape.pack(side="left", fill="x", expand=True)
            cb_shape.bind("<<ComboboxSelected>>", lambda e: [self._prop_set(node, "shape", shape_var.get()), self._refresh_inspector()])

            for key in ("size_x", "size_y", "radius", "height"):
                if key in node["props"]:
                    row = tk.Frame(body, bg=U["card"])
                    row.pack(fill="x", pady=2)
                    tk.Label(row, text=key.replace("_"," ").title(), bg=U["card"], fg=U["fg2"],
                             font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
                    v = tk.StringVar(value=str(node["props"][key]))
                    e = mk_input(row, v, width=10)
                    e.pack(side="left")
                    e.bind("<FocusOut>", lambda ev, k=key, var=v: self._on_float_change(node, k, var.get()))
        else:
            row_poly = tk.Frame(body, bg=U["card"])
            row_poly.pack(fill="x", pady=2)
            tk.Label(row_poly, text="Polygon Points", bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            v_poly = tk.StringVar(value=str(node["props"].get("polygon_points", "")))
            e_poly = mk_input(row_poly, v_poly, width=16)
            e_poly.pack(side="left", fill="x", expand=True)
            e_poly.bind("<FocusOut>", lambda e: self._prop_set(node, "polygon_points", v_poly.get()))

    def _mk_audio_card(self, node):
        body = self._mk_card_frame(node["type"])

        row1 = tk.Frame(body, bg=U["card"])
        row1.pack(fill="x", pady=2)
        tk.Label(row1, text="Audio Stream", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        stream_path = node["props"].get("stream","") or "None (.wav/.mp3)"
        mk_btn(row1, os.path.basename(stream_path) or "Select File",
               lambda: self._pick_audio_stream(node), bg=U["input_bg"], fg=U["fg"],
               padx=6, pady=2, font=("Segoe UI", 8)).pack(side="left", fill="x", expand=True)

        row_bus = tk.Frame(body, bg=U["card"])
        row_bus.pack(fill="x", pady=2)
        tk.Label(row_bus, text="Audio Bus", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        bus_var = tk.StringVar(value=node["props"].get("bus", "Master"))
        cb_bus = ttk.Combobox(row_bus, textvariable=bus_var, style="Unity.TCombobox",
                              values=["Master", "BGM", "SFX", "Voice"], state="readonly", font=("Segoe UI", 8))
        cb_bus.pack(side="left", fill="x", expand=True)
        cb_bus.bind("<<ComboboxSelected>>", lambda e: self._prop_set(node, "bus", bus_var.get()))

        row_vol = tk.Frame(body, bg=U["card"])
        row_vol.pack(fill="x", pady=2)
        tk.Label(row_vol, text="Volume (dB)", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        vol_var = tk.DoubleVar(value=node["props"].get("volume_db", 0.0))
        sc_vol = tk.Scale(row_vol, variable=vol_var, from_=-40, to=12, orient="horizontal",
                          bg=U["card"], fg=U["fg"], highlightthickness=0, troughcolor=U["input_bg"])
        sc_vol.pack(side="left", fill="x", expand=True)
        sc_vol.bind("<ButtonRelease-1>", lambda e: self._prop_set(node, "volume_db", round(vol_var.get(), 1)))

        row_pitch = tk.Frame(body, bg=U["card"])
        row_pitch.pack(fill="x", pady=2)
        tk.Label(row_pitch, text="Pitch Scale", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        pitch_var = tk.DoubleVar(value=node["props"].get("pitch_scale", 1.0))
        sc_pitch = tk.Scale(row_pitch, variable=pitch_var, from_=0.1, to=3.0, resolution=0.1,
                            orient="horizontal", bg=U["card"], fg=U["fg"], highlightthickness=0, troughcolor=U["input_bg"])
        sc_pitch.pack(side="left", fill="x", expand=True)
        sc_pitch.bind("<ButtonRelease-1>", lambda e: self._prop_set(node, "pitch_scale", round(pitch_var.get(), 2)))

        row_toggles = tk.Frame(body, bg=U["card"])
        row_toggles.pack(fill="x", pady=2)
        tk.Label(row_toggles, text="Options", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        for key, lbl in [("autoplay", "Autoplay"), ("loop", "Loop")]:
            var = tk.BooleanVar(value=node["props"].get(key, True))
            tk.Checkbutton(row_toggles, text=lbl, variable=var, bg=U["card"],
                           fg=U["fg"], activebackground=U["card"], selectcolor=U["input_bg"],
                           command=lambda k=key, v=var: self._prop_set(node, k, v.get()),
                           font=("Segoe UI", 8)).pack(side="left", padx=6)

    def _mk_light_card(self, node):
        body = self._mk_card_frame("Light 2D")
        row1 = tk.Frame(body, bg=U["card"])
        row1.pack(fill="x", pady=2)
        tk.Label(row1, text="Color", bg=U["card"], fg=U["fg2"],
                 font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
        col = node["props"].get("color","#ffffaa")
        sw = tk.Label(row1, text="         ", bg=col, relief="solid", bd=1, cursor="hand2")
        sw.pack(side="left")
        sw.bind("<Button-1>", lambda e: self._pick_light_color(node, sw))

        for key in ("radius","energy"):
            row = tk.Frame(body, bg=U["card"])
            row.pack(fill="x", pady=2)
            tk.Label(row, text=key.title(), bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            v = tk.StringVar(value=str(node["props"].get(key,1.0)))
            e = mk_input(row, v, width=10)
            e.pack(side="left")
            e.bind("<FocusOut>", lambda ev, k=key, var=v: self._on_float_change(node, k, var.get()))

    def _mk_generic_card(self, title, node, props):
        body = self._mk_card_frame(title)
        for key, val in props.items():
            row = tk.Frame(body, bg=U["card"])
            row.pack(fill="x", pady=2)
            tk.Label(row, text=key.replace("_"," ").title(), bg=U["card"], fg=U["fg2"],
                     font=("Segoe UI", 8), width=14, anchor="w").pack(side="left")
            if isinstance(val, bool):
                bv = tk.BooleanVar(value=val)
                tk.Checkbutton(row, variable=bv, bg=U["card"], fg=U["fg"],
                               activebackground=U["card"], selectcolor=U["input_bg"],
                               command=lambda k=key, v=bv: self._prop_set(node, k, v.get())).pack(side="left")
            else:
                v = tk.StringVar(value=str(val))
                e = mk_input(row, v, width=14)
                e.pack(side="left", fill="x", expand=True)
                e.bind("<FocusOut>", lambda ev, k=key, var=v, ov=val:
                       self._on_typed_change(node, k, var.get(), type(ov)))

    def _open_add_component_popup(self):
        win = tk.Toplevel(self)
        win.title("Add Component")
        win.geometry("380x440")
        win.configure(bg=U["bg"])
        win.grab_set()

        hdr = tk.Frame(win, bg=U["panel_header"], padx=10, pady=8)
        hdr.pack(fill="x")
        tk.Label(hdr, text="Add Component", font=("Segoe UI", 10, "bold"),
                 bg=U["panel_header"], fg=U["fg"]).pack(side="left")

        search_frame = tk.Frame(win, bg=U["panel"], padx=8, pady=6)
        search_frame.pack(fill="x")
        tk.Label(search_frame, text="🔍", bg=U["panel"], fg=U["fg3"]).pack(side="left")
        search_var = tk.StringVar()
        e_search = mk_input(search_frame, search_var, width=28)
        e_search.pack(side="left", fill="x", expand=True, padx=6)

        list_frame = tk.Frame(win, bg=U["bg"], padx=8, pady=6)
        list_frame.pack(fill="both", expand=True)

        lb = tk.Listbox(list_frame, bg=U["input_bg"], fg=U["fg"], selectbackground=U["selection"],
                        selectforeground="#ffffff", bd=0, font=("Segoe UI", 9), highlightthickness=0)
        sb = ttk.Scrollbar(list_frame, command=lb.yview)
        lb.configure(yscrollcommand=sb.set)
        lb.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")

        all_components = sorted(NODE_TYPES.keys())

        def populate(filter_text=""):
            lb.delete(0, "end")
            for comp in all_components:
                if filter_text.lower() in comp.lower() or filter_text.lower() in NODE_TYPES[comp]["cat"].lower():
                    icon = NODE_TYPES[comp]["icon"]
                    cat = NODE_TYPES[comp]["cat"]
                    lb.insert("end", f" {icon}  {comp}  [{cat}]")

        populate()
        search_var.trace_add("write", lambda *_: populate(search_var.get()))

        def _do_add():
            sel = lb.curselection()
            if not sel: return
            text = lb.get(sel[0]).strip()
            comp_name = text.split()[1]
            if comp_name in NODE_TYPES:
                self._add_node(comp_name)
                win.destroy()

        lb.bind("<Double-1>", lambda e: _do_add())

        bot = tk.Frame(win, bg=U["bg"], pady=8)
        bot.pack(side="bottom", fill="x", padx=10)
        mk_btn(bot, "Cancel", win.destroy, bg=U["panel"], fg=U["fg"]).pack(side="right")
        mk_btn(bot, "Add Component", _do_add, bg=U["accent_blue"], fg="#fff", padx=12).pack(side="right", padx=6)

    # ═══════════════════════════════════════════════════════════════
    # ASSET BROWSER REFRESH & REAL-TIME IMPORT
    # ═══════════════════════════════════════════════════════════════
    def _refresh_asset_browser(self):
        if not hasattr(self, 'folder_tree') or not self.folder_tree.winfo_exists():
            return
        for item in self.folder_tree.get_children():
            self.folder_tree.delete(item)
        self._populate_folder_tree(self.project_path, "")
        self._refresh_icon_grid(self.project_path)

    def _populate_folder_tree(self, path, parent):
        try:
            entries = sorted(os.listdir(path))
        except Exception:
            return
        for name in entries:
            if name.startswith("."): continue
            full = os.path.join(path, name)
            if os.path.isdir(full):
                rel = os.path.relpath(full, self.project_path).replace("\\","/")
                col = self.folder_colors.get(rel,"")
                badge = f" [{col[:7]}]" if col else ""
                iid = self.folder_tree.insert(parent, "end",
                    text=f"📁 {name}{badge}", open=False, iid=full)
                self._populate_folder_tree(full, iid)
            else:
                ext = os.path.splitext(name)[1].lower()
                icon = {"":  "📄", ".png":"🖼️", ".jpg":"🖼️",
                        ".nova":"📜", ".py":"🐍",
                        ".cpp":"⚙️", ".h":"⚙️", ".json":"{}",
                        ".wav":"🎵", ".mp3":"🎵"}.get(ext, "📄")
                self.folder_tree.insert(parent, "end", text=f"{icon} {name}")

    def _on_folder_select(self, event):
        sel = self.folder_tree.selection()
        if sel:
            path = sel[0]
            if os.path.isdir(path):
                self._current_folder = path
                self._refresh_icon_grid(path)

    def _refresh_icon_grid(self, path):
        if not hasattr(self, 'icon_inner') or not self.icon_inner.winfo_exists():
            return
        for w in self.icon_inner.winfo_children():
            w.destroy()
        try:
            entries = sorted(os.listdir(path))
        except Exception:
            return

        col_count = 0
        row_frame = None
        icons_per_row = 6

        for name in entries:
            if name.startswith("."): continue
            full = os.path.join(path, name)
            if col_count % icons_per_row == 0:
                row_frame = tk.Frame(self.icon_inner, bg=U["bg"])
                row_frame.pack(anchor="w", pady=2)
            col_count += 1

            is_dir = os.path.isdir(full)
            ext = os.path.splitext(name)[1].lower()
            icon_text = "📁" if is_dir else {
                ".png":"🖼️",".jpg":"🖼️",".nova":"📜",".py":"🐍",
                ".cpp":"⚙️",".h":"⚙️",".json":"{}",".wav":"🎵",".mp3":"🎵"
            }.get(ext, "📄")

            rel = os.path.relpath(full, self.project_path).replace("\\","/")
            badge_col = self.folder_colors.get(rel, U["bg"]) if is_dir else U["bg"]

            tile = tk.Frame(row_frame, bg=badge_col if is_dir else U["panel"],
                            width=72, height=72, bd=1, relief="solid",
                            cursor="hand2")
            tile.pack(side="left", padx=4)
            tile.pack_propagate(False)

            tk.Label(tile, text=icon_text, font=("Segoe UI", 22),
                     bg=badge_col if is_dir else U["panel"], fg=U["fg"]).pack(pady=(8,0))

            short = name[:9] + ".." if len(name) > 11 else name
            tk.Label(tile, text=short, font=("Segoe UI", 7),
                     bg=badge_col if is_dir else U["panel"], fg=U["fg2"],
                     wraplength=68).pack()

            if is_dir:
                tile.bind("<Double-1>", lambda e, p=full: self._refresh_icon_grid(p))
            else:
                if ext in (".py", ".nova"):
                    tile.bind("<Double-1>", lambda e, p=full: self._open_in_vscode(p))
            tile.bind("<Button-3>", lambda e, iid=full, rd=rel, isd=is_dir:
                      self._show_tile_ctx_menu(e, iid, rd, isd))

    def _show_asset_ctx_menu(self, event):
        m = tk.Menu(self, tearoff=0, bg=U["panel"], fg=U["fg"],
                    activebackground=U["selection"], font=("Segoe UI", 9))
        m.add_command(label="📥 Import Asset...", command=self._import_asset_dialog)
        m.add_command(label="📝 New Python Script", command=lambda: self._new_script_dialog())
        m.add_command(label="📂 Show in Explorer", command=lambda: os.startfile(self._current_folder))
        m.post(event.x_root, event.y_root)

    def _show_tile_ctx_menu(self, event, full_path, rel_path, is_dir):
        m = tk.Menu(self, tearoff=0, bg=U["panel"], fg=U["fg"],
                    activebackground=U["selection"], font=("Segoe UI", 9))
        if is_dir:
            m.add_command(label="🎨 Set Folder Color",
                          command=lambda: self._set_folder_color(rel_path))
            m.add_separator()
        else:
            if full_path.endswith((".py", ".nova")):
                m.add_command(label="📝 Edit in VS Code", command=lambda: self._open_in_vscode(full_path))
                m.add_separator()
        m.add_command(label="📝 New Python Script", command=lambda: self._new_script_dialog())
        m.add_command(label="📂 Open in Explorer",
                      command=lambda: os.startfile(full_path if os.path.isdir(full_path)
                                                   else os.path.dirname(full_path)))
        m.post(event.x_root, event.y_root)

    def _import_asset_dialog(self):
        files = filedialog.askopenfilenames(
            title="Import Asset Files to Project",
            filetypes=[("Images & Audio", "*.png *.jpg *.jpeg *.bmp *.wav *.mp3 *.ogg"), ("All Files", "*.*")])
        if files:
            assets_dir = os.path.join(self.project_path, "assets")
            os.makedirs(assets_dir, exist_ok=True)
            for f in files:
                dest = os.path.join(assets_dir, os.path.basename(f))
                shutil.copy2(f, dest)
            self._refresh_asset_browser()
            messagebox.showinfo("Asset Imported", f"Imported {len(files)} asset(s) to project/assets/")

    def _set_folder_color(self, rel_path):
        color = colorchooser.askcolor(title="Set Folder Color")
        if color and color[1]:
            self.folder_colors[rel_path] = color[1]
            self._save_json(self.colors_db, self.folder_colors)
            self._refresh_asset_browser()

    # ═══════════════════════════════════════════════════════════════
    # 2D VIEWPORT RENDERING & WIREFRAME GIZMOS
    # ═══════════════════════════════════════════════════════════════
    def _render_viewport(self):
        self.canvas.delete("all")
        w = self.canvas.winfo_width()  or 800
        h = self.canvas.winfo_height() or 500
        cx = w / 2 + self.pan_x
        cy = h / 2 + self.pan_y
        z  = self.zoom
        gs = self.grid_size * z

        # Grid
        if gs > 3:
            for x in self._range_steps(cx % gs, w, gs):
                color = U["grid_main"] if abs(x - cx) < 2 else U["grid_line"]
                self.canvas.create_line(x, 0, x, h, fill=color, width=1)
            for y in self._range_steps(cy % gs, h, gs):
                color = U["grid_main"] if abs(y - cy) < 2 else U["grid_line"]
                self.canvas.create_line(0, y, w, y, fill=color, width=1)

        # Origin axes
        self.canvas.create_line(0, cy, w, cy, fill=U["axis_x"], width=1)
        self.canvas.create_line(cx, 0, cx, h, fill=U["axis_y"], width=1)

        # Axis labels
        self.canvas.create_text(cx + 12, 14, text="Y", fill=U["axis_y"],
                                font=("Consolas", 9, "bold"))
        self.canvas.create_text(w - 14, cy - 12, text="X", fill=U["axis_x"],
                                font=("Consolas", 9, "bold"))

        # Nodes
        if self.show_gizmos:
            for node in self.scene_nodes:
                if node["props"].get("visible", True):
                    self._draw_node(node, cx, cy, z)

        # Selection gizmo
        if self.sel_id:
            sel = self._sel_node()
            if sel:
                self._draw_selection_gizmo(sel, cx, cy, z)

    def _world_to_screen(self, wx, wy, cx, cy, z):
        return cx + wx * z, cy + wy * z

    def _screen_to_world(self, sx, sy, cx, cy, z):
        return (sx - cx) / z, (sy - cy) / z

    def _range_steps(self, start, end, step):
        v, out = start, []
        while v < end:
            out.append(v); v += step
        return out

    def _draw_node(self, node, cx, cy, z):
        p = node["props"]
        sx, sy = self._world_to_screen(p.get("position_x", 0), p.get("position_y", 0), cx, cy, z)
        ntype  = node["type"]
        meta   = NODE_TYPES.get(ntype, {})
        col    = meta.get("col", U["fg"])

        if ntype in ("Node2D",):
            r = 5
            self.canvas.create_oval(sx-r, sy-r, sx+r, sy+r, outline=col, width=2)
        elif ntype in ("Sprite2D", "AnimatedSprite2D"):
            mod = p.get("modulate","#ffffff")
            bw  = p.get("width", 64)  * p.get("scale_x", 1.0) * z
            bh  = p.get("height", 64) * p.get("scale_y", 1.0) * z
            tex_path = p.get("texture", "")

            # Attempt real texture rendering
            rendered_tex = False
            if tex_path and os.path.exists(tex_path):
                try:
                    if HAS_PIL:
                        if tex_path not in self.texture_cache:
                            img = Image.open(tex_path)
                            self.texture_cache[tex_path] = img
                        else:
                            img = self.texture_cache[tex_path]
                        resized = img.resize((max(1, int(bw)), max(1, int(bh))), Image.Resampling.NEAREST)
                        photo = ImageTk.PhotoImage(resized)
                        self.canvas.create_image(sx, sy, image=photo)
                        # Retain reference
                        if not hasattr(self, '_photo_refs'): self._photo_refs = []
                        self._photo_refs.append(photo)
                        rendered_tex = True
                except Exception:
                    rendered_tex = False

            if not rendered_tex:
                self.canvas.create_rectangle(sx-bw/2, sy-bh/2, sx+bw/2, sy+bh/2,
                                             fill=mod, outline=col, width=1)
                self.canvas.create_text(sx, sy, text=node["name"], fill="#ffffff",
                                        font=("Segoe UI", 8))

        elif ntype in ("RigidBody2D","StaticBody2D","Area2D","CharacterBody2D"):
            bw = p.get("size_x", 40) * z
            bh = p.get("size_y", 40) * z
            self.canvas.create_rectangle(sx-bw/2, sy-bh/2, sx+bw/2, sy+bh/2,
                                         outline=col, width=2)
            self.canvas.create_text(sx, sy, text=f"{ntype}\n({node['name']})", fill=col, font=("Segoe UI", 7), justify="center")

        elif ntype == "CollisionShape2D":
            shape = p.get("shape", "Box")
            wire_col = U["wireframe_green"]
            if shape == "Circle":
                rad = p.get("radius", 20) * z
                self.canvas.create_oval(sx-rad, sy-rad, sx+rad, sy+rad, outline=wire_col, width=2)
            elif shape == "Capsule":
                rad = p.get("radius", 15) * z
                h_val = p.get("height", 40) * z
                self.canvas.create_rectangle(sx-rad, sy-h_val/2, sx+rad, sy+h_val/2, outline=wire_col, width=2)
                self.canvas.create_oval(sx-rad, sy-h_val/2-rad, sx+rad, sy-h_val/2+rad, outline=wire_col, width=1)
                self.canvas.create_oval(sx-rad, sy+h_val/2-rad, sx+rad, sy+h_val/2+rad, outline=wire_col, width=1)
            else: # Box
                bw = p.get("size_x", 40) * z
                bh = p.get("size_y", 40) * z
                self.canvas.create_rectangle(sx-bw/2, sy-bh/2, sx+bw/2, sy+bh/2, outline=wire_col, width=2)

        elif ntype == "CollisionPolygon2D":
            wire_col = U["wireframe_green"]
            pts_str = p.get("polygon_points", "-20,-20, 20,-20, 20,20, -20,20")
            try:
                coords = [float(c.strip()) for c in pts_str.replace(";",",").split(",") if c.strip()]
                screen_pts = []
                for i in range(0, len(coords)-1, 2):
                    px, py = self._world_to_screen(p.get("position_x",0)+coords[i], p.get("position_y",0)+coords[i+1], cx, cy, z)
                    screen_pts.extend([px, py])
                if len(screen_pts) >= 6:
                    self.canvas.create_polygon(screen_pts, outline=wire_col, fill="", width=2)
            except Exception:
                pass

        elif ntype == "TileMap":
            tiles = p.get("tiles", {})
            cs = p.get("cell_size", 32) * z
            for key, t_id in tiles.items():
                try:
                    gx, gy = map(int, key.split(","))
                    tx = sx + gx * cs
                    ty = sy + gy * cs
                    t_col = next((t["color"] for t in TILE_PALETTE if t["id"] == t_id), "#c8a46e")
                    self.canvas.create_rectangle(tx, ty, tx+cs, ty+cs, fill=t_col, outline="#333", width=1)
                except Exception: pass

        elif ntype in ("AudioStreamPlayer2D", "AudioStreamPlayer"):
            self.canvas.create_text(sx, sy, text="🔊" if ntype=="AudioStreamPlayer2D" else "🎵", font=("Segoe UI", 16))
            self.canvas.create_text(sx, sy+14, text=node["name"], fill="#ff66aa", font=("Segoe UI", 7))
        elif ntype == "Camera2D":
            vw, vh = 180 * z, 100 * z
            self.canvas.create_rectangle(sx-vw/2, sy-vh/2, sx+vw/2, sy+vh/2,
                                         outline=col, dash=(4,4), width=1)
            self.canvas.create_oval(sx-6, sy-6, sx+6, sy+6, outline=col, width=2)
        elif ntype == "Light2D":
            rad = p.get("radius", 150) * z
            lcol = p.get("color", "#ffffaa")
            self.canvas.create_oval(sx-rad, sy-rad, sx+rad, sy+rad,
                                    outline=lcol, dash=(2,2))
            self.canvas.create_oval(sx-7, sy-7, sx+7, sy+7, fill=lcol, outline="#fff")
        elif ntype == "CPUParticles2D":
            sc = p.get("color_start","#ffaa00")
            for i in range(min(p.get("amount",20), 25)):
                dx = math.sin(i * 1.3) * 30 * z
                dy = -math.cos(i * 0.9) * 35 * z
                self.canvas.create_oval(sx+dx-2, sy+dy-2, sx+dx+2, sy+dy+2,
                                        fill=sc, outline="")
        elif ntype in ("Label","Button"):
            bw = p.get("width",80)*z; bh = p.get("height",30)*z
            if ntype == "Button":
                self.canvas.create_rectangle(sx-bw/2, sy-bh/2, sx+bw/2, sy+bh/2,
                                             fill=p.get("color",U["accent_blue"]),
                                             outline="#fff", width=1)
            self.canvas.create_text(sx, sy, text=p.get("text", node["name"]),
                                    fill="#fff", font=("Segoe UI", 9, "bold"))

    def _draw_selection_gizmo(self, node, cx, cy, z):
        p  = node["props"]
        sx, sy = self._world_to_screen(p.get("position_x",0), p.get("position_y",0), cx, cy, z)
        bw = max(p.get("width", p.get("size_x", 48)) * p.get("scale_x",1) * z, 20*z) + 6
        bh = max(p.get("height",p.get("size_y", 48)) * p.get("scale_y",1) * z, 20*z) + 6

        self.canvas.create_rectangle(sx-bw/2, sy-bh/2, sx+bw/2, sy+bh/2,
                                     outline=U["selection"], width=2, dash=(4,4))
        for hx, hy in [(sx-bw/2, sy-bh/2),(sx+bw/2, sy-bh/2),
                       (sx-bw/2, sy+bh/2),(sx+bw/2, sy+bh/2)]:
            self.canvas.create_rectangle(hx-4, hy-4, hx+4, hy+4,
                                         fill=U["selection"], outline="#ffffff", width=1)
        self.canvas.create_oval(sx-5, sy-5, sx+5, sy+5, fill="#ffffff", outline=U["selection"])

        if self.tool == "move":
            self.canvas.create_line(sx, sy, sx+40, sy, fill=U["axis_x"], width=2,
                                    arrow="last", arrowshape=(8,10,4))
            self.canvas.create_line(sx, sy, sx, sy-40, fill=U["axis_y"], width=2,
                                    arrow="last", arrowshape=(8,10,4))

    # ═══════════════════════════════════════════════════════════════
    # CANVAS INTERACTION
    # ═══════════════════════════════════════════════════════════════
    def _cv_click(self, event):
        w = self.canvas.winfo_width() or 1
        h = self.canvas.winfo_height() or 1
        cx, cy = w/2 + self.pan_x, h/2 + self.pan_y
        self.drag_sx, self.drag_sy = event.x, event.y

        if self.tool == "tilemap" or self.tilemap_mode:
            tilemap_node = next((n for n in self.scene_nodes if n["type"] == "TileMap"), None)
            if not tilemap_node:
                self._add_node("TileMap")
                tilemap_node = self._sel_node()
            if tilemap_node:
                wx, wy = self._screen_to_world(event.x, event.y, cx, cy, self.zoom)
                cs = tilemap_node["props"].get("cell_size", 32)
                gx = math.floor(wx / cs)
                gy = math.floor(wy / cs)
                tilemap_node["props"].setdefault("tiles", {})[f"{gx},{gy}"] = self.selected_tile
                self._auto_save_scene()
                self._render_viewport()
                return

        clicked = None
        for node in reversed(self.scene_nodes):
            p = node["props"]
            nx, ny = self._world_to_screen(p.get("position_x",0), p.get("position_y",0), cx, cy, self.zoom)
            bw = max(p.get("width",p.get("size_x",48))*self.zoom,16)
            bh = max(p.get("height",p.get("size_y",48))*self.zoom,16)
            if abs(event.x - nx) <= bw/2+6 and abs(event.y - ny) <= bh/2+6:
                clicked = node
                break

        if clicked:
            self.sel_id = clicked["id"]
            self.drag_orig = (clicked["props"].get("position_x",0),
                              clicked["props"].get("position_y",0))
            self._refresh_hierarchy()
            self._refresh_inspector()
            self._render_viewport()

    def _cv_drag(self, event):
        if self.tool == "tilemap" or self.tilemap_mode:
            self._cv_click(event)
            return

        if not self.sel_id: return
        node = self._sel_node()
        if not node: return
        dx = (event.x - self.drag_sx) / self.zoom
        dy = (event.y - self.drag_sy) / self.zoom

        if self.tool == "move":
            nx = self.drag_orig[0] + dx
            ny = self.drag_orig[1] + dy
            if self.grid_snap:
                gs = self.grid_size
                nx = round(nx/gs)*gs; ny = round(ny/gs)*gs
            node["props"]["position_x"] = round(nx, 2)
            node["props"]["position_y"] = round(ny, 2)
        elif self.tool == "scale":
            node["props"]["scale_x"] = round(max(0.05, 1 + dx*0.01), 3)
            node["props"]["scale_y"] = round(max(0.05, 1 + dy*0.01), 3)
        elif self.tool == "rotate":
            node["props"]["rotation"] = round(dx*2, 1)

        self._refresh_inspector()
        self._render_viewport()
        self._auto_save_scene()

    def _cv_release(self, event):
        pass

    def _pan_start(self, event):
        self._px0 = event.x - self.pan_x
        self._py0 = event.y - self.pan_y

    def _pan_drag(self, event):
        self.pan_x = event.x - self._px0
        self.pan_y = event.y - self._py0
        self._render_viewport()

    def _cv_scroll(self, event):
        factor = 1.15 if event.delta > 0 else 0.87
        self.zoom = round(max(0.1, min(8.0, self.zoom * factor)), 2)
        self.lbl_zoom.configure(text=f"{int(self.zoom*100)}%")
        self._render_viewport()

    def _cv_motion(self, event):
        w = self.canvas.winfo_width() or 1
        h = self.canvas.winfo_height() or 1
        cx, cy = w/2+self.pan_x, h/2+self.pan_y
        wx = round((event.x-cx)/self.zoom, 1)
        wy = round((event.y-cy)/self.zoom, 1)
        self.lbl_cursor.configure(text=f"({wx}, {wy})")

    # ═══════════════════════════════════════════════════════════════
    # TOOL MODE
    # ═══════════════════════════════════════════════════════════════
    def _set_tool(self, mode):
        self.tool = mode
        self.tilemap_mode = (mode == "tilemap")
        for m, b in self._tool_btns.items():
            b.configure(bg=U["btn_active"] if m == mode else U["btn_normal"],
                        fg="#ffffff" if m == mode else U["fg"])
        if hasattr(self, 'tile_drawer'):
            if mode == "tilemap":
                self.tile_drawer.pack(fill="x", before=self.canvas)
            else:
                self.tile_drawer.pack_forget()

    # ═══════════════════════════════════════════════════════════════
    # VIEWPORT TAB SWITCHING
    # ═══════════════════════════════════════════════════════════════
    def _switch_scene_tab(self):
        self._scene_tab_active = True
        self.btn_scene_tab.configure(bg=U["tab_active"], fg=U["fg"])
        self.btn_game_tab.configure(bg=U["tab_inactive"], fg=U["fg2"])

    def _switch_game_tab(self):
        self._scene_tab_active = False
        self.btn_game_tab.configure(bg=U["tab_active"], fg=U["fg"])
        self.btn_scene_tab.configure(bg=U["tab_inactive"], fg=U["fg2"])

    def _toggle_2d(self):
        self.show_2d = not self.show_2d
        self._btn_2d.configure(bg=U["btn_active"] if self.show_2d else U["btn_normal"])

    def _toggle_gizmos(self):
        self.show_gizmos = not self.show_gizmos
        self._btn_gizmos.configure(bg=U["btn_active"] if self.show_gizmos else U["btn_normal"])
        self._render_viewport()

    # ═══════════════════════════════════════════════════════════════
    # NODE MANAGEMENT
    # ═══════════════════════════════════════════════════════════════
    def _show_add_node_menu(self):
        m = tk.Menu(self, tearoff=0, bg=U["panel"], fg=U["fg"],
                    activebackground=U["selection"], font=("Segoe UI", 9))
        cats = {}
        for ntype, meta in NODE_TYPES.items():
            cat = meta["cat"]
            cats.setdefault(cat, []).append((ntype, meta))

        for cat, entries in cats.items():
            sub = tk.Menu(m, tearoff=0, bg=U["panel"], fg=U["fg"],
                          activebackground=U["selection"], font=("Segoe UI", 9))
            for ntype, meta in entries:
                sub.add_command(label=f"{meta['icon']}  {ntype}",
                                command=lambda t=ntype: self._add_node(t))
            m.add_cascade(label=f"▶  {cat}", menu=sub)
        m.post(self.winfo_pointerx(), self.winfo_pointery())

    def _add_node(self, ntype):
        new_id   = f"node_{int(time.time()*1000)}"
        count    = sum(1 for n in self.scene_nodes if n["type"]==ntype) + 1
        props    = NODE_TYPES.get(ntype,{}).get("props",{}).copy()
        parent   = self.sel_id or ("node_root" if self.scene_nodes else None)
        new_node = {"id":new_id,"name":f"{ntype}_{count}","type":ntype,"parent":parent,
                    "tag":"Untagged","layer":"Default","props":props}
        self.scene_nodes.append(new_node)
        self.sel_id = new_id
        self._refresh_hierarchy()
        self._refresh_inspector()
        self._render_viewport()
        self._auto_save_scene()

    def _duplicate_node(self):
        node = self._sel_node()
        if not node or node["id"] == "node_root": return
        new_id = f"node_{int(time.time()*1000)}"
        dup = {**node, "id":new_id, "name":node["name"]+"_Copy",
               "props":{**node["props"], "position_x":node["props"].get("position_x",0)+20,
                        "position_y":node["props"].get("position_y",0)+20}}
        self.scene_nodes.append(dup)
        self.sel_id = new_id
        self._refresh_hierarchy()
        self._refresh_inspector()
        self._render_viewport()
        self._auto_save_scene()

    def _delete_node(self):
        if not self.sel_id or self.sel_id == "node_root":
            return
        self.scene_nodes = [n for n in self.scene_nodes
                            if n["id"] != self.sel_id and n.get("parent") != self.sel_id]
        self.sel_id = "node_root"
        self._refresh_hierarchy()
        self._refresh_inspector()
        self._render_viewport()
        self._auto_save_scene()

    def _sel_node(self):
        return next((n for n in self.scene_nodes if n["id"] == self.sel_id), None)

    # ═══════════════════════════════════════════════════════════════
    # PROPERTY HELPERS & AUTO SAVE
    # ═══════════════════════════════════════════════════════════════
    def _prop_set(self, node, key, val):
        node["props"][key] = val
        self._render_viewport()
        self._auto_save_scene()

    def _on_name_change(self, node, val):
        node["name"] = val.strip() or node["name"]
        self._refresh_hierarchy()
        self._auto_save_scene()

    def _on_float_change(self, node, key, text):
        try: node["props"][key] = round(float(text), 4)
        except ValueError: pass
        self._render_viewport()
        self._auto_save_scene()

    def _on_typed_change(self, node, key, text, typ):
        try:
            node["props"][key] = (int(text) if typ==int else
                                  float(text) if typ==float else text)
        except ValueError: pass
        self._render_viewport()
        self._auto_save_scene()

    def _pick_texture(self, node):
        path = filedialog.askopenfilename(
            title="Select Sprite Texture",
            filetypes=[("Images","*.png *.jpg *.jpeg *.bmp"), ("All Files","*.*")])
        if path:
            # Copy to project assets folder for clean structure
            dest_dir = os.path.join(self.project_path, "assets")
            os.makedirs(dest_dir, exist_ok=True)
            dest_path = os.path.join(dest_dir, os.path.basename(path))
            if path != dest_path:
                shutil.copy2(path, dest_path)
            node["props"]["texture"] = dest_path
            self.texture_cache.pop(dest_path, None)
            self._refresh_inspector()
            self._render_viewport()
            self._auto_save_scene()

    def _pick_audio_stream(self, node):
        path = filedialog.askopenfilename(
            title="Select Audio Stream",
            filetypes=[("Audio Files","*.wav *.mp3 *.ogg"), ("All Files","*.*")])
        if path:
            dest_dir = os.path.join(self.project_path, "assets")
            os.makedirs(dest_dir, exist_ok=True)
            dest_path = os.path.join(dest_dir, os.path.basename(path))
            if path != dest_path:
                shutil.copy2(path, dest_path)
            node["props"]["stream"] = dest_path
            self._refresh_inspector()
            self._auto_save_scene()

    def _pick_modulate(self, node, swatch):
        col = colorchooser.askcolor(initialcolor=node["props"].get("modulate","#ffffff"))
        if col and col[1]:
            node["props"]["modulate"] = col[1]
            swatch.configure(bg=col[1])
            self._render_viewport()
            self._auto_save_scene()

    def _pick_light_color(self, node, swatch):
        col = colorchooser.askcolor(initialcolor=node["props"].get("color","#ffffaa"))
        if col and col[1]:
            node["props"]["color"] = col[1]
            swatch.configure(bg=col[1])
            self._render_viewport()
            self._auto_save_scene()

    # ═══════════════════════════════════════════════════════════════
    # 1️⃣ PLAY ENGINE & SCENE ISOLATION BUG FIX
    # ═══════════════════════════════════════════════════════════════
    def _on_play(self):
        """Export scene to <project>/scenes/main.json and launch MoxEngine.exe."""
        # 1. Guarantee the scenes/ directory exists
        scenes_dir = os.path.join(self.project_path, "scenes")
        os.makedirs(scenes_dir, exist_ok=True)

        # 2. Write current scene state directly to scenes/main.json
        self._save_scene()

        self.btn_play.configure(bg=U["play_active"], fg="#ffffff")

        # 3. Locate MoxEngine.exe
        exe = self._find_exe()
        if not exe:
            messagebox.showwarning(
                "Engine Required",
                "MoxEngine.exe not found.\n"
                "Expected at: Engine/build/MoxEngine.exe\n"
                "Please run HUB/compile_engine.bat first.")
            self.btn_play.configure(bg=U["bg"], fg=U["fg2"])
            return

        # 4. Kill previous instance if still running
        if self.engine_proc and self.engine_proc.poll() is None:
            try:
                self.engine_proc.terminate()
            except Exception:
                pass

        # 5. Build command with absolute forward-slash paths
        abs_project = os.path.abspath(self.project_path).replace("\\", "/")
        abs_exe     = os.path.abspath(exe).replace("\\", "/")
        rel_scene   = "scenes/main.json"

        cmd = [abs_exe, "--project", abs_project, "--scene", rel_scene]

        # 6. Debug log so the user can see exactly what is launched
        print("[Mox Play] Launching engine:")
        print(f"  EXE     : {abs_exe}")
        print(f"  PROJECT : {abs_project}")
        print(f"  SCENE   : {rel_scene}")
        print(f"  CMD     : {' '.join(cmd)}")

        try:
            self.engine_proc = subprocess.Popen(
                cmd, cwd=os.path.dirname(abs_exe))
        except Exception as e:
            messagebox.showerror("Engine Launch Error",
                f"Failed to run game binary:\n{abs_exe}\n\n{e}")
            self.btn_play.configure(bg=U["bg"], fg=U["fg2"])

    def _on_pause(self):
        self.btn_pause.configure(
            bg=U["play_active"] if self.btn_pause.cget("bg") != U["play_active"] else U["bg"])

    def _on_step(self):
        pass

    def _find_exe(self):
        """Search for MoxEngine.exe relative to HUB directory."""
        hub_dir = os.path.dirname(os.path.abspath(__file__))
        root_dir = os.path.dirname(hub_dir)   # one level up from HUB/

        candidates = [
            # Primary: Engine/build/ next to HUB/
            os.path.join(root_dir, "Engine", "build", "MoxEngine.exe"),
            # Installed version
            os.path.join(root_dir, "Engine", "versions", "Mox Engine1", "MoxEngine.exe"),
            # Legacy: engines/ subfolder inside HUB/
            os.path.join(hub_dir, "engines", "MoxEngine1", "MoxEngine.exe"),
            # Fallback: project-local binary
            os.path.join(self.project_path, "MoxEngine.exe"),
        ]
        for p in candidates:
            resolved = os.path.normpath(p)
            if os.path.exists(resolved):
                print(f"[Mox Play] Found engine at: {resolved}")
                return resolved

        print("[Mox Play] Engine not found. Searched:")
        for p in candidates:
            print(f"  {os.path.normpath(p)}")
        return None

    # ═══════════════════════════════════════════════════════════════
    # 2️⃣ VS CODE INTEGRATION & PYTHON SCRIPTING
    # ═══════════════════════════════════════════════════════════════
    def _open_in_vscode(self, target_path):
        """Open a file or folder in VS Code with direct exe path discovery."""
        # Ensure scripts directory exists
        os.makedirs(os.path.join(self.project_path, "scripts"), exist_ok=True)

        # If the target is a .py path that doesn't exist yet → create default template
        if target_path and target_path.endswith(".py") and not os.path.exists(target_path):
            os.makedirs(os.path.dirname(target_path), exist_ok=True)
            class_name = os.path.splitext(os.path.basename(target_path))[0]
            class_name = ''.join(w.capitalize() for w in class_name.replace('-','_').split('_'))
            template = (
                f"# {os.path.basename(target_path)}\n"
                f"# Auto-generated by Mox Engine HUB\n\n"
                f"import mox\n\n"
                f"class {class_name}(mox.CharacterBody2D):\n"
                f"    def _ready(self):\n"
                f"        pass\n\n"
                f"    def _process(self, delta):\n"
                f"        pass\n"
            )
            try:
                with open(target_path, "w", encoding="utf-8") as f:
                    f.write(template)
                print(f"[Mox Script] Created template: {target_path}")
            except Exception as err:
                print(f"[Mox Script] Could not create template: {err}")

        # Fallback: if path still doesn't exist use project folder
        if not target_path or not os.path.exists(target_path):
            target_path = self.project_path

        abs_path = os.path.abspath(target_path)
        print(f"[Mox Script] Target script path: {abs_path}")

        # Search for Code.exe in standard Windows install paths
        candidates = [
            os.path.join(os.path.expanduser("~"), r"AppData\Local\Programs\Microsoft VS Code\Code.exe"),
            r"C:\Program Files\Microsoft VS Code\Code.exe",
            r"C:\Program Files (x86)\Microsoft VS Code\Code.exe",
        ]
        
        vscode_exe = next((p for p in candidates if os.path.exists(p)), None)

        if vscode_exe:
            print(f"[Mox Script] Found direct VS Code executable: {vscode_exe}")
            try:
                subprocess.Popen([vscode_exe, abs_path])
                return
            except Exception as e:
                print(f"[Mox Script] Direct VS Code launch failed ({e}), falling back...")

        # Try system code command via shell
        try:
            subprocess.Popen(["code", abs_path], shell=True)
            return
        except Exception as e:
            print(f"[Mox Script] System code command failed: {e}")

        # Fallback: native OS file opener
        try:
            if os.path.isfile(abs_path):
                os.startfile(abs_path)   # type: ignore[attr-defined]
            else:
                os.startfile(abs_path)
        except Exception as e2:
            print(f"[Mox Script] os.startfile failed: {e2}")
            if os.path.isfile(abs_path):
                self._open_script_editor()

    def _new_script_dialog(self, node=None):
        win = tk.Toplevel(self); win.title("New Python Script")
        win.geometry("440x240"); win.configure(bg=U["bg"])
        win.resizable(False,False); win.grab_set()

        tk.Label(win, text="📝  Create Python Script", font=("Segoe UI",12,"bold"),
                 bg=U["bg"], fg=U["fg"]).pack(pady=(16,4), padx=20, anchor="w")

        r1 = tk.Frame(win, bg=U["bg"], padx=20); r1.pack(fill="x", pady=5)
        tk.Label(r1, text="Script Name:", width=12, anchor="w",
                 bg=U["bg"], fg=U["fg2"], font=("Segoe UI",9)).pack(side="left")
        default_name = f"{node['name'].lower()}_script" if node else "player_script"
        nv = tk.StringVar(value=default_name)
        mk_input(r1, nv, width=22).pack(side="left", fill="x", expand=True)

        r2 = tk.Frame(win, bg=U["bg"], padx=20); r2.pack(fill="x", pady=5)
        tk.Label(r2, text="Language:", width=12, anchor="w",
                 bg=U["bg"], fg=U["fg2"], font=("Segoe UI",9)).pack(side="left")
        lv = tk.StringVar(value="Python (.py)")
        ttk.Combobox(r2, textvariable=lv, style="Unity.TCombobox",
                     values=["Python (.py)", "NovaScript (.nova)", "C++ Header (.h)"],
                     state="readonly", width=22).pack(side="left")

        def _create():
            name = nv.get().strip()
            if not name: return
            ext_map = {"Python (.py)":".py","NovaScript (.nova)":".nova","C++ Header (.h)":".h"}
            ext = ext_map.get(lv.get(),".py")
            if not name.endswith(ext): name += ext

            spath = os.path.join(self.project_path, "scripts", name)
            os.makedirs(os.path.dirname(spath), exist_ok=True)

            # Generate standard Python template for node
            node_class = node["type"] if node else "Node2D"
            py_template = f"# {name}\n# Attached to: {node['name'] if node else 'Node'}\n\nimport mox\n\nclass Script(mox.{node_class}):\n    def _ready(self):\n        print(\"{name} ready!\")\n\n    def _process(self, delta):\n        pass\n"

            with open(spath, "w", encoding="utf-8") as f:
                f.write(py_template)

            if node:
                node["props"]["script"] = spath
                self._refresh_inspector()
                self._auto_save_scene()

            self._refresh_asset_browser()
            win.destroy()
            self._open_in_vscode(spath)

        bot = tk.Frame(win, bg=U["bg"], pady=10); bot.pack(side="bottom", fill="x", padx=20)
        mk_btn(bot,"Cancel",win.destroy,bg=U["panel"],fg=U["fg"]).pack(side="right")
        mk_btn(bot,"✅ Create & Edit in VS Code",_create,bg=U["accent_blue"],fg="#fff",padx=14).pack(side="right",padx=8)

    # ═══════════════════════════════════════════════════════════════
    # 3️⃣ SCENE SERIALIZATION & AUTO SAVE
    # ═══════════════════════════════════════════════════════════════
    def _load_scene(self):
        data = self._load_json(self.scene_file, {})
        if not data:
            # Fallback check main_scene.json
            fallback = os.path.join(self.project_path, "main_scene.json")
            data = self._load_json(fallback, {})

        self.scene_nodes = data.get("scene", [])
        if not self.scene_nodes:
            self.scene_nodes = [
                {"id":"node_root","name":"MainScene","type":"Node2D","parent":None,
                 "tag":"Untagged","layer":"Default",
                 "props":NODE_TYPES["Node2D"]["props"].copy()}
            ]
        self.sel_id = self.scene_nodes[0]["id"]

    def _save_scene(self):
        os.makedirs(os.path.join(self.project_path, "scenes"), exist_ok=True)
        scene_data = {"version":"4.0","scene":self.scene_nodes}
        self._save_json(self.scene_file, scene_data)
        # Also save to main_scene.json for backwards compatibility
        self._save_json(os.path.join(self.project_path, "main_scene.json"), scene_data)

    def _auto_save_scene(self):
        self._save_scene()

    def _new_scene(self):
        if not messagebox.askyesno("New Scene","Create empty scene?\nUnsaved changes will be lost."): return
        self.scene_nodes = [{"id":"node_root","name":"MainScene","type":"Node2D",
                             "parent":None,"tag":"Untagged","layer":"Default",
                             "props":NODE_TYPES["Node2D"]["props"].copy()}]
        self.sel_id = "node_root"
        self._refresh_hierarchy(); self._refresh_inspector(); self._render_viewport()
        self._auto_save_scene()

    def _open_scene(self):
        path = filedialog.askopenfilename(initialdir=self.project_path,
            title="Open Scene", filetypes=[("Mox Scene","*.json"),("All Files","*.*")])
        if not path: return
        try:
            data = self._load_json(path, {})
            self.scene_nodes = data.get("scene", [])
            self.sel_id = self.scene_nodes[0]["id"] if self.scene_nodes else None
            self._refresh_hierarchy(); self._refresh_inspector(); self._render_viewport()
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def _build_project(self):
        bat = os.path.join(os.path.dirname(__file__), "compile_engine.bat")
        if not os.path.exists(bat):
            messagebox.showerror("Error", f"compile_engine.bat not found:\n{bat}"); return
        subprocess.Popen([bat], shell=True)
        messagebox.showinfo("Build", "Engine build started.")

    # ═══════════════════════════════════════════════════════════════
    # MENU ACTIONS & UTILITIES
    # ═══════════════════════════════════════════════════════════════
    def _show_project_settings(self):
        win = tk.Toplevel(self)
        win.title("Project Settings"); win.geometry("480x340")
        win.configure(bg=U["bg"]); win.grab_set()

        tk.Label(win, text="⚙  Project Settings", font=("Segoe UI",13,"bold"),
                 bg=U["bg"], fg=U["fg"]).pack(pady=(16,4), padx=20, anchor="w")

        cfg_path = os.path.join(self.project_path, "project.json")
        s = self._load_json(cfg_path, {"title":"My Mox Game","width":1280,"height":720,
                                        "gravity":980.0,"render_api":"OpenGL"})
        vars_ = {}
        for lbl, key, typ in [("Game Title","title",str),("Window Width","width",int),
                               ("Window Height","height",int),
                               ("Gravity (px/s²)","gravity",float),("Render API","render_api",str)]:
            row = tk.Frame(win, bg=U["card"], padx=12, pady=7)
            row.pack(fill="x", padx=20, pady=2)
            tk.Label(row, text=lbl, width=18, anchor="w",
                     bg=U["card"], fg=U["fg2"], font=("Segoe UI",9)).pack(side="left")
            v = tk.StringVar(value=str(s.get(key,"")))
            mk_input(row, v, width=20).pack(side="left", fill="x", expand=True)
            vars_[key] = (v, typ)

        def _save():
            for key,(v,typ) in vars_.items():
                try: s[key] = typ(v.get())
                except ValueError: pass
            self._save_json(cfg_path, s)
            messagebox.showinfo("Saved","Project settings saved.", parent=win)
            win.destroy()

        bot = tk.Frame(win, bg=U["bg"], pady=10); bot.pack(side="bottom", fill="x", padx=20)
        mk_btn(bot,"Cancel",win.destroy,bg=U["panel"],fg=U["fg"]).pack(side="right")
        mk_btn(bot,"Save",_save,bg=U["selection"],fg="#fff",padx=14).pack(side="right",padx=8)

    def _open_script_editor(self):
        win = tk.Toplevel(self); win.title("Built-in Script Editor")
        win.geometry("720x520"); win.configure(bg=U["bg"])

        hdr = tk.Frame(win, bg=U["panel_header"], padx=8, pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="📝  Script Editor", font=("Segoe UI",11,"bold"),
                 bg=U["panel_header"], fg=U["fg"]).pack(side="left")

        scripts = []
        for root, _, files in os.walk(self.project_path):
            for f in files:
                if f.endswith((".py",".nova",".cpp",".h")):
                    scripts.append(os.path.join(root, f))

        script_var = tk.StringVar()
        cb = ttk.Combobox(hdr, textvariable=script_var, values=scripts,
                          style="Unity.TCombobox", width=36, state="readonly")
        cb.pack(side="left", padx=8)

        text_area = tk.Text(win, bg="#1e1e1e", fg="#d4d4d4", insertbackground="#fff",
                            font=("Consolas",10), bd=0, padx=10, pady=8, undo=True)
        text_area.pack(fill="both", expand=True, padx=4, pady=4)

        def _load(path):
            try:
                with open(path,"r",encoding="utf-8") as f:
                    text_area.delete("1.0","end"); text_area.insert("1.0",f.read())
            except Exception as e: messagebox.showerror("Error",str(e),parent=win)

        def _save_s():
            p = script_var.get()
            if not p: return
            try:
                with open(p,"w",encoding="utf-8") as f:
                    f.write(text_area.get("1.0","end"))
                messagebox.showinfo("Saved",f"Saved: {os.path.basename(p)}",parent=win)
            except Exception as e: messagebox.showerror("Error",str(e),parent=win)

        cb.bind("<<ComboboxSelected>>", lambda e: _load(script_var.get()))
        if scripts: cb.current(0); _load(scripts[0])

        mk_btn(hdr, "📝 Open VS Code", lambda: self._open_in_vscode(script_var.get()),
               bg=U["btn_normal"], fg=U["accent_blue"]).pack(side="right", padx=4)
        mk_btn(hdr, "💾 Save", _save_s, bg=U["selection"], fg="#fff").pack(side="right")

    def _recompile_engine(self):
        bat = os.path.join(os.path.dirname(__file__), "compile_engine.bat")
        if not os.path.exists(bat):
            messagebox.showerror("Error", f"compile_engine.bat not found:\n{bat}"); return
        if messagebox.askyesno("Recompile","Start C++ engine recompilation?"):
            subprocess.Popen([bat], shell=True)

    def _load_json(self, path, default):
        try:
            with open(path,"r",encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            return default

    def _save_json(self, path, data):
        try:
            with open(path,"w",encoding="utf-8") as f:
                json.dump(data, f, indent=2)
        except Exception as e:
            messagebox.showerror("Save Error", str(e))


# ═══════════════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Mox Engine 2D Editor")
    parser.add_argument("project", nargs="?", default=None)
    parser.add_argument("--project", dest="project_flag", default=None)
    args = parser.parse_args()
    proj_path = args.project_flag or args.project or None
    app = MoxEditor(project_path=proj_path)
    app.mainloop()
