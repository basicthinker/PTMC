; ModuleID = 'std-string.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define weak i8* @tanger_stm_std_memset(i8* %src, i32 %c, i64 %n) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %src}, i64 0, metadata !20), !dbg !23
  tail call void @llvm.dbg.value(metadata !{i32 %c}, i64 0, metadata !21), !dbg !23
  tail call void @llvm.dbg.value(metadata !{i64 %n}, i64 0, metadata !22), !dbg !23
  tail call void @_ITM_memsetW(i8* %src, i32 %c, i64 %n) nounwind, !dbg !24
  ret i8* %src, !dbg !26
}

define weak i8* @tanger_stm_std_memcpy(i8* %dest, i8* %src, i64 %n) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %dest}, i64 0, metadata !17), !dbg !27
  tail call void @llvm.dbg.value(metadata !{i8* %src}, i64 0, metadata !18), !dbg !27
  tail call void @llvm.dbg.value(metadata !{i64 %n}, i64 0, metadata !19), !dbg !27
  tail call void @_ITM_memcpyRtWt(i8* %dest, i8* %src, i64 %n) nounwind, !dbg !28
  ret i8* %dest, !dbg !30
}

define weak i8* @tanger_stm_std_memmove(i8* %dest, i8* %src, i64 %n) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %dest}, i64 0, metadata !14), !dbg !31
  tail call void @llvm.dbg.value(metadata !{i8* %src}, i64 0, metadata !15), !dbg !31
  tail call void @llvm.dbg.value(metadata !{i64 %n}, i64 0, metadata !16), !dbg !31
  tail call void @_ITM_memmoveRtWt(i8* %dest, i8* %src, i64 %n) nounwind, !dbg !32
  ret i8* %dest, !dbg !34
}

declare void @_ITM_memmoveRtWt(i8*, i8*, i64)

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

declare void @_ITM_memcpyRtWt(i8*, i8*, i64)

declare void @_ITM_memsetW(i8*, i32, i64)

!llvm.dbg.sp = !{!0, !9, !10}
!llvm.dbg.lv.tanger_stm_std_memmove = !{!14, !15, !16}
!llvm.dbg.lv.tanger_stm_std_memcpy = !{!17, !18, !19}
!llvm.dbg.lv.tanger_stm_std_memset = !{!20, !21, !22}

!0 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_std_memmove", metadata !"tanger_stm_std_memmove", metadata !"tanger_stm_std_memmove", metadata !1, i32 32, metadata !3, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*, i8*, i64)* @tanger_stm_std_memmove} ; [ DW_TAG_subprogram ]
!1 = metadata !{i32 524329, metadata !"std-string.c", metadata !"/opt/tanger/37936/src/lib/stmsupport/", metadata !2} ; [ DW_TAG_file_type ]
!2 = metadata !{i32 524305, i32 0, i32 1, metadata !"std-string.c", metadata !"/opt/tanger/37936/src/lib/stmsupport/", metadata !"4.2.1 (Based on Apple Inc. build 5658) (LLVM build)", i1 true, i1 true, metadata !"", i32 0} ; [ DW_TAG_compile_unit ]
!3 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !4, i32 0, null} ; [ DW_TAG_subroutine_type ]
!4 = metadata !{metadata !5, metadata !5, metadata !5, metadata !6}
!5 = metadata !{i32 524303, metadata !1, metadata !"", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, null} ; [ DW_TAG_pointer_type ]
!6 = metadata !{i32 524310, metadata !7, metadata !"size_t", metadata !7, i32 29, i64 0, i64 0, i64 0, i32 0, metadata !8} ; [ DW_TAG_typedef ]
!7 = metadata !{i32 524329, metadata !"xlocale.h", metadata !"/usr/include", metadata !2} ; [ DW_TAG_file_type ]
!8 = metadata !{i32 524324, metadata !1, metadata !"long unsigned int", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ]
!9 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_std_memcpy", metadata !"tanger_stm_std_memcpy", metadata !"tanger_stm_std_memcpy", metadata !1, i32 25, metadata !3, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*, i8*, i64)* @tanger_stm_std_memcpy} ; [ DW_TAG_subprogram ]
!10 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_std_memset", metadata !"tanger_stm_std_memset", metadata !"tanger_stm_std_memset", metadata !1, i32 18, metadata !11, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*, i32, i64)* @tanger_stm_std_memset} ; [ DW_TAG_subprogram ]
!11 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !12, i32 0, null} ; [ DW_TAG_subroutine_type ]
!12 = metadata !{metadata !5, metadata !5, metadata !13, metadata !6}
!13 = metadata !{i32 524324, metadata !1, metadata !"int", metadata !1, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ]
!14 = metadata !{i32 524545, metadata !0, metadata !"dest", metadata !1, i32 31, metadata !5} ; [ DW_TAG_arg_variable ]
!15 = metadata !{i32 524545, metadata !0, metadata !"src", metadata !1, i32 31, metadata !5} ; [ DW_TAG_arg_variable ]
!16 = metadata !{i32 524545, metadata !0, metadata !"n", metadata !1, i32 31, metadata !6} ; [ DW_TAG_arg_variable ]
!17 = metadata !{i32 524545, metadata !9, metadata !"dest", metadata !1, i32 24, metadata !5} ; [ DW_TAG_arg_variable ]
!18 = metadata !{i32 524545, metadata !9, metadata !"src", metadata !1, i32 24, metadata !5} ; [ DW_TAG_arg_variable ]
!19 = metadata !{i32 524545, metadata !9, metadata !"n", metadata !1, i32 24, metadata !6} ; [ DW_TAG_arg_variable ]
!20 = metadata !{i32 524545, metadata !10, metadata !"src", metadata !1, i32 17, metadata !5} ; [ DW_TAG_arg_variable ]
!21 = metadata !{i32 524545, metadata !10, metadata !"c", metadata !1, i32 17, metadata !13} ; [ DW_TAG_arg_variable ]
!22 = metadata !{i32 524545, metadata !10, metadata !"n", metadata !1, i32 17, metadata !6} ; [ DW_TAG_arg_variable ]
!23 = metadata !{i32 17, i32 0, metadata !10, null}
!24 = metadata !{i32 19, i32 0, metadata !25, null}
!25 = metadata !{i32 524299, metadata !10, i32 18, i32 0, metadata !1, i32 2} ; [ DW_TAG_lexical_block ]
!26 = metadata !{i32 20, i32 0, metadata !25, null}
!27 = metadata !{i32 24, i32 0, metadata !9, null}
!28 = metadata !{i32 26, i32 0, metadata !29, null}
!29 = metadata !{i32 524299, metadata !9, i32 25, i32 0, metadata !1, i32 1} ; [ DW_TAG_lexical_block ]
!30 = metadata !{i32 27, i32 0, metadata !29, null}
!31 = metadata !{i32 31, i32 0, metadata !0, null}
!32 = metadata !{i32 33, i32 0, metadata !33, null}
!33 = metadata !{i32 524299, metadata !0, i32 32, i32 0, metadata !1, i32 0} ; [ DW_TAG_lexical_block ]
!34 = metadata !{i32 34, i32 0, metadata !33, null}
