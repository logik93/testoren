#!/usr/bin/env python3
"""Render the English Markdown manual to PDF (requires reportlab)."""
from pathlib import Path
import re
from xml.sax.saxutils import escape
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Image

root = Path(__file__).resolve().parents[1]
for name, file in [('OpenSans', 'OpenSans-Regular.ttf'), ('OpenSans-Bold', 'OpenSans-Semibold.ttf')]:
    pdfmetrics.registerFont(TTFont(name, str(root / 'assets/fonts' / file)))
pdfmetrics.registerFontFamily('OpenSans', normal='OpenSans', bold='OpenSans-Bold')
styles = getSampleStyleSheet()
for style in styles.byName.values():
    style.fontName = 'OpenSans'
    style.textColor = colors.HexColor('#283632')
styles['Normal'].fontSize = 9
styles['Normal'].leading = 14
styles['Normal'].spaceAfter = 8
for name in ['Title', 'Heading1', 'Heading2', 'Heading3']:
    styles[name].fontName = 'OpenSans-Bold'
styles.add(ParagraphStyle('Cell', parent=styles['Normal'], fontSize=8, leading=11, spaceAfter=0))

def inline(text):
    return re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', escape(text))

lines = (root / 'docs/MANUAL.md').read_text().splitlines()
version = re.search(r'logisth ([0-9.]+)', lines[0]).group(1)
story, para, table = [], [], []
def flush():
    if para:
        story.append(Paragraph(inline(' '.join(para)), styles['Normal']))
        para.clear()
def flush_table():
    if not table:
        return
    count = len(table[0])
    widths = [165 * 2.83465 / count] * count
    if count == 2:
        widths = [100, 368]
    obj = Table([[Paragraph(inline(c), styles['Cell']) for c in row] for row in table],
                colWidths=widths, repeatRows=1, hAlign='LEFT')
    obj.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#e3ebe1')),
        ('GRID', (0, 0), (-1, -1), .3, colors.HexColor('#ced2c9')),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
        ('LEFTPADDING', (0, 0), (-1, -1), 6),
        ('RIGHTPADDING', (0, 0), (-1, -1), 6),
        ('TOPPADDING', (0, 0), (-1, -1), 6),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 6)]))
    story.extend([obj, Spacer(1, 12)])
    table.clear()
for line in lines + ['']:
    if line.startswith('|'):
        flush()
        cells = [c.strip() for c in line.strip('|').split('|')]
        if not all(re.fullmatch(r'[- :]+', c) for c in cells):
            table.append(cells)
        continue
    flush_table()
    if not line.strip():
        flush()
    elif line.startswith('# '):
        flush()
        story.append(Paragraph(inline(line[2:]), styles['Title']))
        story.append(Image(str(root / 'docs/images/logisth-dark.png'), width=468, height=468*860/1240))
        story.append(Spacer(1, 16))
    elif line.startswith('### '):
        flush()
        story.append(Paragraph(inline(line[4:]), styles['Heading3']))
    elif re.match(r'^\d+\. ', line):
        flush()
        story.append(Paragraph(inline(line), styles['Normal']))
    elif line.startswith('## '):
        flush()
        story.append(Paragraph(inline(line[3:]), styles['Heading2']))
    else:
        para.append(line.strip())

def footer(canvas, doc):
    canvas.setFont('OpenSans', 8)
    canvas.setFillColor(colors.HexColor('#667268'))
    canvas.drawString(42, 25, f'logisth {version} | English User Manual')
    canvas.drawRightString(A4[0]-42, 25, str(doc.page))

SimpleDocTemplate(str(root / 'docs/logisth-Manual-English.pdf'), pagesize=A4,
                  rightMargin=42, leftMargin=42, topMargin=42, bottomMargin=44,
                  title=f'logisth {version} — User Manual', author='Testoren').build(
                      story, onFirstPage=footer, onLaterPages=footer)
print('Rendered docs/logisth-Manual-English.pdf')
