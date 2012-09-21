/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
/* Plugin / Public Interface */

#ifndef PATCHAPI_COMMAND_H_
#define PATCHAPI_COMMAND_H_

#include "PatchCommon.h"

namespace Dyninst {
namespace PatchAPI {

/* Interface to support transactional semantics, by implementing an
   instrumentation request (public interface) or an internal step of
   instrumentation (plugin interface) */

class Command {
  public:
    Command() {}
    virtual ~Command() {}

    PATCHAPI_EXPORT virtual bool commit();

    PATCHAPI_EXPORT virtual bool run() = 0;
    PATCHAPI_EXPORT virtual bool undo() = 0;
};

/* A BatchCommand is in fact a list of Commands, and is to iterate all Commands
   in the list to run() or undo(). */

class BatchCommand : public Command {
  public:
    PATCHAPI_EXPORT BatchCommand* create();
    BatchCommand() {}
    virtual ~BatchCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();

    /* Add/Remove Commands to to_do_ list. */
    typedef std::list<Command*> CommandList;
    PATCHAPI_EXPORT void add(Command*);
    PATCHAPI_EXPORT void remove(CommandList::iterator);

  protected:
    CommandList to_do_;
    CommandList done_;

};

/* A Patcher is a special BatchCommand, which implicitly execute Instrumenter
   after executing all Commands in its list. Instrumenter is for code relocation
   and code generation. */

class Patcher : public BatchCommand {
  public:
   PATCHAPI_EXPORT static Patcher* create(Dyninst::PatchAPI::PatchMgrPtr mgr) {
      return new Patcher(mgr);
    }
    Patcher(Dyninst::PatchAPI::PatchMgrPtr mgr) : mgr_(mgr) {}
    virtual ~Patcher() {}

    PATCHAPI_EXPORT virtual bool run();
  private:
    Dyninst::PatchAPI::PatchMgrPtr mgr_;
};

/* Default implementation of some basic instrumentation Commands */

class PushFrontCommand : public Command {
  public:
    static PushFrontCommand* create(Dyninst::PatchAPI::Point* pt,
                      Dyninst::PatchAPI::SnippetPtr snip) {
      return new PushFrontCommand(pt, snip);
    }
    PushFrontCommand(Dyninst::PatchAPI::Point* pt,
                     Dyninst::PatchAPI::SnippetPtr snip) : pt_(pt), snip_(snip) {}
    virtual ~PushFrontCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();
    InstancePtr instance() { return instance_; }
 private:
   Dyninst::PatchAPI::Point* pt_;
   Dyninst::PatchAPI::SnippetPtr snip_;
   Dyninst::PatchAPI::InstancePtr instance_;
};

class PushBackCommand : public Command {
  public:
    static PushBackCommand* create(Dyninst::PatchAPI::Point* pt,
                      Dyninst::PatchAPI::SnippetPtr snip) {
      return new PushBackCommand(pt, snip);
    }
    PushBackCommand(Dyninst::PatchAPI::Point* pt,
                    Dyninst::PatchAPI::SnippetPtr snip)
                    : pt_(pt), snip_(snip) {}
    virtual ~PushBackCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();
    InstancePtr instance() { return instance_; }

  private:
    Dyninst::PatchAPI::Point* pt_;
    Dyninst::PatchAPI::SnippetPtr snip_;
    Dyninst::PatchAPI::InstancePtr instance_;
};

class RemoveSnippetCommand : public Command {
  public:
    static RemoveSnippetCommand* create(Dyninst::PatchAPI::InstancePtr instance) {
      return new RemoveSnippetCommand(instance);
    }
    RemoveSnippetCommand(Dyninst::PatchAPI::InstancePtr instance)
      : instance_(instance) {}
    virtual ~RemoveSnippetCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();
  private:
    Dyninst::PatchAPI::InstancePtr instance_;
};

class RemoveCallCommand : public Command {
  public:
    static RemoveCallCommand* create(Dyninst::PatchAPI::PatchMgrPtr mgr,
                      Dyninst::PatchAPI::PatchBlock* call_block,
                      Dyninst::PatchAPI::PatchFunction* context = NULL) {
      return new RemoveCallCommand(mgr, call_block, context);
    }
    RemoveCallCommand(Dyninst::PatchAPI::PatchMgrPtr mgr,
                      Dyninst::PatchAPI::PatchBlock* call_block,
                      Dyninst::PatchAPI::PatchFunction* context)
      : mgr_(mgr), call_block_(call_block), context_(context) {}
    virtual ~RemoveCallCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();
  private:
    Dyninst::PatchAPI::PatchMgrPtr mgr_;
    Dyninst::PatchAPI::PatchBlock* call_block_;
    Dyninst::PatchAPI::PatchFunction* context_;
};

class ReplaceCallCommand : public Command {
  public:
    static ReplaceCallCommand* create(Dyninst::PatchAPI::PatchMgrPtr mgr,
                      Dyninst::PatchAPI::PatchBlock* call_block,
                      Dyninst::PatchAPI::PatchFunction* new_callee,
                      Dyninst::PatchAPI::PatchFunction* context) {
      return new ReplaceCallCommand(mgr, call_block, new_callee, context);
    }
    ReplaceCallCommand(Dyninst::PatchAPI::PatchMgrPtr mgr,
                       Dyninst::PatchAPI::PatchBlock* call_block,
                       Dyninst::PatchAPI::PatchFunction* new_callee,
                       Dyninst::PatchAPI::PatchFunction* context)
      : mgr_(mgr), call_block_(call_block), new_callee_(new_callee), context_(context) {}
    virtual ~ReplaceCallCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();
  private:
    Dyninst::PatchAPI::PatchMgrPtr mgr_;
    Dyninst::PatchAPI::PatchBlock* call_block_;
    Dyninst::PatchAPI::PatchFunction* new_callee_;
    Dyninst::PatchAPI::PatchFunction* context_;
};

class ReplaceFuncCommand : public Command {
  public:
    static ReplaceFuncCommand* create(Dyninst::PatchAPI::PatchMgrPtr mgr,
                      Dyninst::PatchAPI::PatchFunction* old_func,
                      Dyninst::PatchAPI::PatchFunction* new_func) {
      return new ReplaceFuncCommand(mgr, old_func, new_func);
    }
    ReplaceFuncCommand(Dyninst::PatchAPI::PatchMgrPtr mgr,
                       Dyninst::PatchAPI::PatchFunction* old_func,
                       Dyninst::PatchAPI::PatchFunction* new_func)
      : mgr_(mgr), old_func_(old_func), new_func_(new_func)  {}
    virtual ~ReplaceFuncCommand() {}

    PATCHAPI_EXPORT virtual bool run();
    PATCHAPI_EXPORT virtual bool undo();
  private:
    Dyninst::PatchAPI::PatchMgrPtr mgr_;
    Dyninst::PatchAPI::PatchFunction* old_func_;
    Dyninst::PatchAPI::PatchFunction* new_func_;
};

}
}

#endif /* PATCHAPI_COMMAND_H_ */