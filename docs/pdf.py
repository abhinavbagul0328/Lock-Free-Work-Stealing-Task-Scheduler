from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, HRFlowable, KeepTogether
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY, TA_RIGHT
from reportlab.graphics.shapes import Drawing, Rect, String, Line, Polygon, Group

OUTPUT = "LockFree_TaskScheduler_Submission.pdf"

W, H = A4
MARGIN = 12*mm

# ── B&W Palette ───────────────────────────────────────────────────────────────
BLACK      = colors.HexColor("#000000")
DARK       = colors.HexColor("#1A1A1A")
DGRAY      = colors.HexColor("#333333")
MGRAY      = colors.HexColor("#666666")
LGRAY      = colors.HexColor("#AAAAAA")
XLIGHT     = colors.HexColor("#E8E8E8")
XXLIGHT    = colors.HexColor("#F5F5F5")
WHITE      = colors.white

doc = SimpleDocTemplate(
    OUTPUT, pagesize=A4,
    leftMargin=MARGIN, rightMargin=MARGIN,
    topMargin=8*mm, bottomMargin=8*mm
)

CW = W - 2*MARGIN  # content width

# ── Styles ────────────────────────────────────────────────────────────────────
def S(name, **kw):
    return ParagraphStyle(name, **kw)

sTITLE  = S("title",  fontName="Helvetica-Bold", fontSize=15, textColor=WHITE,  leading=18)
sNAME   = S("name",   fontName="Helvetica",      fontSize=9,  textColor=XLIGHT, leading=12)
sSEC    = S("sec",    fontName="Helvetica-Bold",  fontSize=8,  textColor=WHITE,  leading=10)
sBODY   = S("body",   fontName="Helvetica",       fontSize=7.8,textColor=DARK,   leading=11, alignment=TA_JUSTIFY)
sBODYC  = S("bodyc",  fontName="Helvetica",       fontSize=7.8,textColor=DARK,   leading=11, alignment=TA_CENTER)
sBODYB  = S("bodyb",  fontName="Helvetica-Bold",  fontSize=7.8,textColor=DARK,   leading=11)
sBODYBW = S("bodybw", fontName="Helvetica-Bold",  fontSize=7.8,textColor=WHITE,  leading=11)
sHDR    = S("hdr",    fontName="Helvetica-Bold",  fontSize=7.5,textColor=WHITE,  leading=10)
sHDRC   = S("hdrc",   fontName="Helvetica-Bold",  fontSize=7.5,textColor=WHITE,  leading=10, alignment=TA_CENTER)
sSMALL  = S("small",  fontName="Helvetica",       fontSize=7,  textColor=MGRAY,  leading=9)
sSMALLB = S("smallb", fontName="Helvetica-Bold",  fontSize=7,  textColor=DARK,   leading=9)
sGAIN   = S("gain",   fontName="Helvetica-Bold",  fontSize=8,  textColor=DARK,   leading=10, alignment=TA_CENTER)
sCENTW  = S("centw",  fontName="Helvetica",       fontSize=7.8,textColor=WHITE,  leading=11, alignment=TA_CENTER)
sCENTBW = S("centbw", fontName="Helvetica-Bold",  fontSize=7.8,textColor=WHITE,  leading=11, alignment=TA_CENTER)

def sp(h=1.5): return Spacer(1, h*mm)

def sec_bar(title):
    t = Table([[Paragraph(title, sSEC)]], colWidths=[CW])
    t.setStyle(TableStyle([
        ("BACKGROUND",    (0,0),(-1,-1), BLACK),
        ("LEFTPADDING",   (0,0),(-1,-1), 7),
        ("TOPPADDING",    (0,0),(-1,-1), 3.5),
        ("BOTTOMPADDING", (0,0),(-1,-1), 3.5),
    ]))
    return t

story = []

# ═══════════════════════════════════════════════════════════════════════════════
# HEADER — title + name only, black bg
# ═══════════════════════════════════════════════════════════════════════════════
hdr = Table([[
    Paragraph("Lock-Free Work-Stealing Task Scheduler", sTITLE),
    Paragraph("Deven Bagul<br/>Modern C++20 • Concurrency • Lock-Free", sNAME),
]], colWidths=[CW*0.65, CW*0.35])
hdr.setStyle(TableStyle([
    ("BACKGROUND",    (0,0),(-1,-1), BLACK),
    ("LEFTPADDING",   (0,0),(-1,-1), 8),
    ("RIGHTPADDING",  (0,0),(-1,-1), 8),
    ("TOPPADDING",    (0,0),(-1,-1), 7),
    ("BOTTOMPADDING", (0,0),(-1,-1), 7),
    ("VALIGN",        (0,0),(-1,-1), "MIDDLE"),
    ("ALIGN",         (1,0),(1,0),   "RIGHT"),
]))
story.append(hdr)
story.append(sp(2.5))

# ═══════════════════════════════════════════════════════════════════════════════
# CONTEXT — tight paragraph
# ═══════════════════════════════════════════════════════════════════════════════
story.append(sec_bar("▸  CONTEXT"))
story.append(sp(1.5))
story.append(Paragraph(
    "Traditional thread pools suffer from massive performance degradation due to centralized lock contention and cache-line bouncing. "
    "I built a high-performance <b>C++ runtime engine</b> that distributes heterogeneous workloads across multiple CPU cores entirely "
    "without mutexes. It auto-balances irregular, recursive tasks via a distributed <b>Chase-Lev work-stealing deque</b> architecture, "
    "reducing thread synchronization overhead and achieving blazing fast execution. The runtime is heavily optimized to prevent "
    "false sharing and maximize L1 cache locality for highly parallel CPU-bound pipelines.",
    sBODY
))
story.append(sp(2.5))

# ═══════════════════════════════════════════════════════════════════════════════
# DATA STRUCTURES USED
# ═══════════════════════════════════════════════════════════════════════════════
story.append(sec_bar("▸ DATA STRUCTURES & THEIR ROLE IN OPTIMIZATION"))
story.append(sp(1.5))

ds_headers = [
    Paragraph("Data Structure", sHDR),
    Paragraph("Where Used", sHDR),
    Paragraph("Why Chosen / Optimization Impact", sHDR),
    Paragraph("Complexity", sHDRC),
]

ds_rows = [
    [
        "Chase-Lev Work-Stealing Deque\n(std::vector + atomics)",
        "Local task queue per\nworker thread",
        "Owner thread pushes/pops from the bottom (LIFO) for maximum cache locality. "
        "Idle threads steal from the top (FIFO). Completely eliminates centralized lock contention.",
        "O(1) pop\nO(1) steal",
    ],
    [
        "Cache-Aligned Atomics\n(alignas(64) std::atomic)",
        "Deque top/bottom indices\nand thread metrics",
        "Padded critical state variables to 64-byte boundaries. Prevents 'false sharing' "
        "across worker threads and completely eliminates cache-coherency ping-pong traffic.",
        "O(1) access\nZero stalls",
    ],
    [
        "Type-Erased Callable Wrapper\n(std::function<void()>)",
        "Heterogeneous task\nabstraction layer",
        "Allows dynamic execution of arbitrary workloads without virtual function dispatch overhead. "
        "Leverages Small Object Optimization (SOO) to eliminate heap allocations for captured state.",
        "O(1) invoke\nO(1) alloc",
    ],
    [
        "Thread-Local Context Pointers\n(thread_local Worker*)",
        "Implicit recursive task\nrouting mechanism",
        "Allows heavily recursive tasks (like Fibonacci) to seamlessly spawn sub-tasks directly "
        "into the local worker's deque without passing explicit scheduler context downwards.",
        "O(1) resolve\nZero locks",
    ],
    [
        "Contention-Free Wait Loop\n(std::this_thread::yield)",
        "Sub-task completion\nsynchronization",
        "Threads never block the OS while waiting for children. Instead, they actively execute "
        "other local tasks or attempt to steal work, drastically increasing CPU saturation.",
        "Active Spin\nNon-blocking",
    ],
]

col_w1 = [CW*0.24, CW*0.18, CW*0.44, CW*0.14]
ds_data = [ds_headers]
for i, row in enumerate(ds_rows):
    bg = WHITE if i % 2 == 0 else XXLIGHT
    ds_data.append([Paragraph(cell.replace("\n","<br/>"), sBODY) for cell in row])

ds_table = Table(ds_data, colWidths=col_w1, repeatRows=1)
ds_table.setStyle(TableStyle([
    ("BACKGROUND",    (0,0),(-1,0),  BLACK),
    ("TEXTCOLOR",     (0,0),(-1,0),  WHITE),
    ("ROWBACKGROUNDS",(0,1),(-1,-1), [WHITE, XXLIGHT]),
    ("GRID",          (0,0),(-1,-1), 0.4, LGRAY),
    ("LINEBELOW",     (0,0),(-1,0),  1.2, BLACK),
    ("LEFTPADDING",   (0,0),(-1,-1), 5),
    ("RIGHTPADDING",  (0,0),(-1,-1), 5),
    ("TOPPADDING",    (0,0),(-1,-1), 3),
    ("BOTTOMPADDING", (0,0),(-1,-1), 3),
    ("VALIGN",        (0,0),(-1,-1), "TOP"),
    ("ALIGN",         (3,0),(3,-1),  "CENTER"),
    ("FONTNAME",      (0,1),(0,-1),  "Helvetica-Bold"),
    ("FONTSIZE",      (0,1),(0,-1),  7.5),
]))
story.append(ds_table)
story.append(sp(2.5))

# ═══════════════════════════════════════════════════════════════════════════════
# MEASURABLE IMPACT
# ═══════════════════════════════════════════════════════════════════════════════
story.append(sec_bar("▸ MEASURABLE IMPACT"))
story.append(sp(1.5))

imp_headers = [
    Paragraph("Optimization Area", sHDR),
    Paragraph("Technique Applied", sHDR),
    Paragraph("Baseline", sHDRC),
    Paragraph("Optimized", sHDRC),
    Paragraph("Gain", sHDRC),
    Paragraph("Benchmark", sHDRC),
]

imp_rows = [
    ["Queue Synchronization",        "C++20 Atomics with\nmemory_order_release", "2.81s",   "0.10s",  "28.1×", "5M Pushes/Pops"],
    ["False Sharing Prevention",     "alignas(64) padding\non atomic indices",   "388 ms",  "54 ms",  "7.1×",  "100M Increments"],
    ["Dynamic Load Balancing",       "Distributed Deques\n+ Randomized Stealing","1.85s",   "0.30s",  "6.1×",  "Unbalanced QSort"],
    ["Recursive Task Spawning",      "Implicit Routing via\nthread_local",       "126 ns",  "0 ns",   "100% ↓","Task Allocations"],
    ["Massive Parallel Compute",     "LIFO Local Execution\nfor cache locality", "3.26s",   "0.91s",  "3.5×",  "MatMul 1024x1024"],
    ["Uneven Workloads",             "Active Spin-Wait\nwith continuous Steal",  "2.96s",   "0.30s",  "9.8×",  "QuickSort 10M"],
]

col_w2 = [CW*0.20, CW*0.23, CW*0.13, CW*0.12, CW*0.11, CW*0.21]
imp_data = [imp_headers]
for row in imp_rows:
    imp_data.append([Paragraph(cell.replace("\n","<br/>"), sBODY) for cell in row])

imp_table = Table(imp_data, colWidths=col_w2, repeatRows=1)
imp_table.setStyle(TableStyle([
    ("BACKGROUND",    (0,0),(-1,0),  BLACK),
    ("TEXTCOLOR",     (0,0),(-1,0),  WHITE),
    ("ROWBACKGROUNDS",(0,1),(-1,-1), [WHITE, XXLIGHT]),
    ("GRID",          (0,0),(-1,-1), 0.4, LGRAY),
    ("LINEBELOW",     (0,0),(-1,0),  1.2, BLACK),
    ("LEFTPADDING",   (0,0),(-1,-1), 5),
    ("RIGHTPADDING",  (0,0),(-1,-1), 5),
    ("TOPPADDING",    (0,0),(-1,-1), 3),
    ("BOTTOMPADDING", (0,0),(-1,-1), 3),
    ("VALIGN",        (0,0),(-1,-1), "TOP"),
    ("ALIGN",         (2,0),(5,-1),  "CENTER"),
    ("BACKGROUND",    (4,1),(4,-1),  XLIGHT),
    ("FONTNAME",      (4,1),(4,-1),  "Helvetica-Bold"),
    ("FONTSIZE",      (4,1),(4,-1),  8),
    ("FONTNAME",      (0,1),(0,-1),  "Helvetica-Bold"),
    ("FONTSIZE",      (0,1),(0,-1),  7.5),
]))
story.append(imp_table)
story.append(sp(2.5))

# ═══════════════════════════════════════════════════════════════════════════════
# ARCHITECTURE DIAGRAM
# ═══════════════════════════════════════════════════════════════════════════════
story.append(sec_bar("▸  SYSTEM ARCHITECTURE PIPELINE"))
story.append(sp(1.5))

drawing = Drawing(CW, 100)

def draw_box(x, y, w, h, lines, fill_color, text_color):
    g = Group()
    g.add(Rect(x, y, w, h, fillColor=fill_color, strokeColor=MGRAY, strokeWidth=1, rx=4, ry=4))
    line_h = 10
    total_h = len(lines) * line_h
    start_y = y + (h - total_h)/2 + (len(lines)-1)*line_h
    for i, line in enumerate(lines):
        g.add(String(x + w/2, start_y - i*line_h - 1, line, fontName="Helvetica-Bold", fontSize=8, fillColor=text_color, textAnchor="middle"))
    return g

def draw_arrow(x1, y1, x2, y2, bidirectional=False):
    g = Group()
    g.add(Line(x1, y1, x2, y2, strokeColor=MGRAY, strokeWidth=1.5))
    g.add(Polygon([x2, y2, x2-5, y2+4, x2-5, y2-4], fillColor=MGRAY, strokeColor=None))
    if bidirectional:
        g.add(Polygon([x1, y1, x1+5, y1+4, x1+5, y1-4], fillColor=MGRAY, strokeColor=None))
    return g

b1 = draw_box(5, 30, 100, 40, ["Global Scheduler", "(Root Task Queue)"], WHITE, BLACK)
b2 = draw_box(135, 60, 120, 35, ["Worker 1", "(Local LIFO Execution)"], XLIGHT, BLACK)
b3 = draw_box(135, 10, 120, 35, ["Worker 2", "(Local LIFO Execution)"], XLIGHT, BLACK)
b4 = draw_box(305, 35, 120, 40, ["Idle Worker 3", "(FIFO Stealing Mode)"], BLACK, WHITE)

# Arrows from global to workers
a1 = draw_arrow(105, 50, 135, 75)
a2 = draw_arrow(105, 50, 135, 25)

# Stealing arrows
a3 = draw_arrow(305, 55, 255, 75)
a4 = draw_arrow(305, 55, 255, 25)

drawing.add(b1)
drawing.add(b2)
drawing.add(b3)
drawing.add(b4)
drawing.add(a1)
drawing.add(a2)
drawing.add(a3)
drawing.add(a4)

story.append(drawing)
story.append(sp(2.5))

# ═══════════════════════════════════════════════════════════════════════════════
# FOOTER BAR
# ═══════════════════════════════════════════════════════════════════════════════
footer = Table([[
    Paragraph("Stack: <b>Modern C++20 · CMake · pthreads · std::atomic</b>", sSMALL),
    Paragraph("Algorithm: <b>Lock-Free Programming · Chase-Lev Deque · Work Stealing</b>", sSMALL),
]], colWidths=[CW*0.45, CW*0.55])
footer.setStyle(TableStyle([
    ("BACKGROUND",    (0,0),(-1,-1), DARK),
    ("LEFTPADDING",   (0,0),(-1,-1), 7),
    ("RIGHTPADDING",  (0,0),(-1,-1), 7),
    ("TOPPADDING",    (0,0),(-1,-1), 5),
    ("BOTTOMPADDING", (0,0),(-1,-1), 5),
    ("TEXTCOLOR",     (0,0),(-1,-1), LGRAY),
    ("ALIGN",         (1,0),(1,0),   "RIGHT"),
]))
story.append(footer)

doc.build(story)
print("Done:", OUTPUT)