; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout =
"e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-s0:0:64-f80:32:32"
target triple = "i686-pc-linux-gnu"
deplibs = [ "m", "tanger-stm", "uuid", "rt", "crypt", "pthread", "dl" ]
    %struct.apr_hash_entry_t = type { %struct.apr_hash_entry_t*, i32, i8*,
i32, i8* }
    %struct.cache_hash_index_t = type { %struct.cache_hash_t*,
%struct.apr_hash_entry_t*, %struct.apr_hash_entry_t*, i32 }
    %struct.cache_hash_t = type { %struct.apr_hash_entry_t**,
%struct.cache_hash_index_t, i32, i32 }

define %struct.cache_hash_t* @cache_hash_make(i32 %size) {
entry:
    br i1 false, label %cond_true34, label %UnifiedReturnBlock

cond_true34:        ; preds = %entry
    free [7 x %struct.apr_hash_entry_t**]* null
    ret %struct.cache_hash_t* null

UnifiedReturnBlock:     ; preds = %entry
    ret %struct.cache_hash_t* null
}
