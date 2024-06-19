//
// Created by PikachuHy on 2021/11/5.
//

#ifndef QTMARKDOWN_DOCUMENT_H
#define QTMARKDOWN_DOCUMENT_H
#include "QtMarkdown_global.h"
#include "mddef.h"
#include "parser/Document.h"
#include "render/Instruction.h"
#include "render/Render.h"
#include "Cursor.h"
#include "CursorCoord.h"

namespace md::editor {
class CommandStack;
class Cursor;
struct CursorCoord;
class QTMARKDOWNSHARED_EXPORT Document : public parser::Document, public std::enable_shared_from_this<Document> {
 public:
  explicit Document(const String& str, sptr<render::RenderSetting> setting);
  CursorCoord moveCursorToRight(CursorCoord coord);
  CursorCoord moveCursorToLeft(CursorCoord coord);
  CursorCoord moveCursorToBol(CursorCoord coord);
  std::pair<CursorCoord, int> moveCursorToEol(CursorCoord coord);
  CursorCoord moveCursorToUp(CursorCoord coord, Point pos);
  CursorCoord moveCursorToDown(CursorCoord coord, Point pos);
  CursorCoord moveCursorToPos(Point pos);
  CursorCoord moveCursorToBeginOfDocument();
  CursorCoord moveCursorToEndOfDocument();
  void insertText(Cursor& cursor, const String& text);
  void removeText(Cursor& cursor);
  void insertReturn(Cursor& cursor);
  const render::BlockList& blocks() const { return m_blocks; };

  void renderAllBlock();
  void replaceBlock(SizeType blockNo, parser::Node* node);
  void insertBlock(SizeType blockNo, parser::Node* node);
  void renderBlock(SizeType blockNo);
  void removeBlock(SizeType blockNo);
  void mergeBlock(SizeType blockNo1, SizeType blockNo2);

  void undo(Cursor& cursor);
  void redo(Cursor& cursor);

  void upgradeToHeader(const Cursor& cursor, int level);

  parser::Container* node2container(parser::Node* node);
  // FIXME: 在win上使用编译的dll错误
  /*
   Command.cpp.obj : error LNK2019: 无法解析的外部符号
   "public: void __cdecl md::editor::Document::updateCursor(class md::editor::Cursor &,class md::editor::CursorCoord
   const &,bool)" (?updateCursor@Document@editor@md@@QEAAXAEAVCursor@23@AEBVCursorCoord@23@_N@Z)， 函数 "public: virtual
   void __cdecl md::editor::InsertTextCommand::execute(class md::editor::Cursor &)"
   (?execute@InsertTextCommand@editor@md@@UEAAXAEAVCursor@23@@Z) 中引用了该符号
   */
  void updateCursor(Cursor& cursor, const CursorCoord& coord, bool updatePos = true);
  std::pair<Point, int> mapToScreen(const CursorCoord& coord);
  bool isBol(const CursorCoord& coord) const;

  render::BlockList m_blocks;
  sptr<render::RenderSetting> m_setting;
  sptr<CommandStack> m_commandStack;
  friend class Editor;
  friend class EditorTest;
  friend class DocumentOperationVisitor;
  friend class InsertReturnVisitor;
  friend class RemoveTextVisitor;
  friend class InsertTextVisitor;
  friend class InsertTextCommand;
  friend class RemoveTextCommand;
  friend class InsertReturnCommand;
};
}  // namespace md::editor

#endif  // QTMARKDOWN_DOCUMENT_H
