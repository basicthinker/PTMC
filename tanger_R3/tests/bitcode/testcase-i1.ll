; ModuleID = 'calls-combined.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-s0:0:64-f80:32:32"
target triple = "i686-pc-linux-gnu"
@shutdown_pending.b = weak global i1 0
@is_graceful = weak global i32 0

define internal void @sig_term(i32 %sig) {
entry:
    %tmp3.b = load i1* @shutdown_pending.b, align 1     ; <i1> [#uses=1]
    br i1 %tmp3.b, label %UnifiedReturnBlock, label %cond_next

cond_next:      ; preds = %entry
    %tmp1 = icmp eq i32 %sig, 28        ; <i1> [#uses=1]
    %tmp12 = zext i1 %tmp1 to i32       ; <i32> [#uses=1]
    store i1 true, i1* @shutdown_pending.b, align 1
    volatile store i32 %tmp12, i32* @is_graceful, align 4
    ret void

UnifiedReturnBlock:     ; preds = %entry
    ret void
}
