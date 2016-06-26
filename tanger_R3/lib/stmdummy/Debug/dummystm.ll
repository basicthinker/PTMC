; ModuleID = 'dummystm.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define noalias i8* @tanger_stm_report_start(i8* nocapture %name) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !38), !dbg !62
  ret i8* null, !dbg !63
}

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

define noalias i8* @tanger_stm_report_start_object(i8* nocapture %handle, i8* nocapture %name) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !39), !dbg !65
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !40), !dbg !65
  ret i8* null, !dbg !66
}

define noalias i8* @tanger_stm_report_start_array(i8* nocapture %handle, i8* nocapture %name) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !41), !dbg !68
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !42), !dbg !68
  ret i8* null, !dbg !69
}

define void @tanger_stm_report_append_long(i8* nocapture %handle, i8* nocapture %name, i64 %i) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !43), !dbg !71
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !44), !dbg !71
  tail call void @llvm.dbg.value(metadata !{i64 %i}, i64 0, metadata !45), !dbg !71
  ret void, !dbg !72
}

define void @tanger_stm_report_append_int(i8* nocapture %handle, i8* nocapture %name, i32 %i) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !46), !dbg !74
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !47), !dbg !74
  tail call void @llvm.dbg.value(metadata !{i32 %i}, i64 0, metadata !48), !dbg !74
  ret void, !dbg !75
}

define void @tanger_stm_report_append_bool(i8* nocapture %handle, i8* nocapture %name, i32 %i) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !49), !dbg !77
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !50), !dbg !77
  tail call void @llvm.dbg.value(metadata !{i32 %i}, i64 0, metadata !51), !dbg !77
  ret void, !dbg !78
}

define void @tanger_stm_report_append_double(i8* nocapture %handle, i8* nocapture %name, double %d) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !52), !dbg !80
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !53), !dbg !80
  tail call void @llvm.dbg.value(metadata !{double %d}, i64 0, metadata !54), !dbg !80
  ret void, !dbg !81
}

define void @tanger_stm_report_append_string(i8* nocapture %handle, i8* nocapture %name, i8* nocapture %str) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !55), !dbg !83
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !56), !dbg !83
  tail call void @llvm.dbg.value(metadata !{i8* %str}, i64 0, metadata !57), !dbg !83
  ret void, !dbg !84
}

define void @tanger_stm_report_finish_object(i8* nocapture %handle) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !58), !dbg !86
  ret void, !dbg !87
}

define void @tanger_stm_report_finish_array(i8* nocapture %handle) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !59), !dbg !89
  ret void, !dbg !90
}

define void @tanger_stm_report_finish(i8* nocapture %handle, i8* nocapture %name) nounwind readnone {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %handle}, i64 0, metadata !60), !dbg !92
  tail call void @llvm.dbg.value(metadata !{i8* %name}, i64 0, metadata !61), !dbg !92
  ret void, !dbg !93
}

!llvm.dbg.sp = !{!0, !9, !12, !13, !19, !23, !24, !28, !31, !34, !35}
!llvm.dbg.lv.tanger_stm_report_start = !{!38}
!llvm.dbg.lv.tanger_stm_report_start_object = !{!39, !40}
!llvm.dbg.lv.tanger_stm_report_start_array = !{!41, !42}
!llvm.dbg.lv.tanger_stm_report_append_long = !{!43, !44, !45}
!llvm.dbg.lv.tanger_stm_report_append_int = !{!46, !47, !48}
!llvm.dbg.lv.tanger_stm_report_append_bool = !{!49, !50, !51}
!llvm.dbg.lv.tanger_stm_report_append_double = !{!52, !53, !54}
!llvm.dbg.lv.tanger_stm_report_append_string = !{!55, !56, !57}
!llvm.dbg.lv.tanger_stm_report_finish_object = !{!58}
!llvm.dbg.lv.tanger_stm_report_finish_array = !{!59}
!llvm.dbg.lv.tanger_stm_report_finish = !{!60, !61}

!0 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_start", metadata !"tanger_stm_report_start", metadata !"tanger_stm_report_start", metadata !1, i32 25, metadata !3, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*)* @tanger_stm_report_start} ; [ DW_TAG_subprogram ]
!1 = metadata !{i32 524329, metadata !"dummystm.c", metadata !"/opt/tanger/37936/src/lib/stmdummy/", metadata !2} ; [ DW_TAG_file_type ]
!2 = metadata !{i32 524305, i32 0, i32 1, metadata !"dummystm.c", metadata !"/opt/tanger/37936/src/lib/stmdummy/", metadata !"4.2.1 (Based on Apple Inc. build 5658) (LLVM build)", i1 true, i1 true, metadata !"", i32 0} ; [ DW_TAG_compile_unit ]
!3 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !4, i32 0, null} ; [ DW_TAG_subroutine_type ]
!4 = metadata !{metadata !5, metadata !6}
!5 = metadata !{i32 524303, metadata !1, metadata !"", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, null} ; [ DW_TAG_pointer_type ]
!6 = metadata !{i32 524303, metadata !1, metadata !"", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !7} ; [ DW_TAG_pointer_type ]
!7 = metadata !{i32 524326, metadata !1, metadata !"", metadata !1, i32 0, i64 8, i64 8, i64 0, i32 0, metadata !8} ; [ DW_TAG_const_type ]
!8 = metadata !{i32 524324, metadata !1, metadata !"char", metadata !1, i32 0, i64 8, i64 8, i64 0, i32 0, i32 6} ; [ DW_TAG_base_type ]
!9 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_start_object", metadata !"tanger_stm_report_start_object", metadata !"tanger_stm_report_start_object", metadata !1, i32 26, metadata !10, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*, i8*)* @tanger_stm_report_start_object} ; [ DW_TAG_subprogram ]
!10 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !11, i32 0, null} ; [ DW_TAG_subroutine_type ]
!11 = metadata !{metadata !5, metadata !5, metadata !6}
!12 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_start_array", metadata !"tanger_stm_report_start_array", metadata !"tanger_stm_report_start_array", metadata !1, i32 27, metadata !10, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*, i8*)* @tanger_stm_report_start_array} ; [ DW_TAG_subprogram ]
!13 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_append_long", metadata !"tanger_stm_report_append_long", metadata !"tanger_stm_report_append_long", metadata !1, i32 28, metadata !14, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*, i64)* @tanger_stm_report_append_long} ; [ DW_TAG_subprogram ]
!14 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !15, i32 0, null} ; [ DW_TAG_subroutine_type ]
!15 = metadata !{null, metadata !5, metadata !6, metadata !16}
!16 = metadata !{i32 524310, metadata !17, metadata !"int64_t", metadata !17, i32 49, i64 0, i64 0, i64 0, i32 0, metadata !18} ; [ DW_TAG_typedef ]
!17 = metadata !{i32 524329, metadata !"stdint.h", metadata !"/usr/include", metadata !2} ; [ DW_TAG_file_type ]
!18 = metadata !{i32 524324, metadata !1, metadata !"long int", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ]
!19 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_append_int", metadata !"tanger_stm_report_append_int", metadata !"tanger_stm_report_append_int", metadata !1, i32 29, metadata !20, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*, i32)* @tanger_stm_report_append_int} ; [ DW_TAG_subprogram ]
!20 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !21, i32 0, null} ; [ DW_TAG_subroutine_type ]
!21 = metadata !{null, metadata !5, metadata !6, metadata !22}
!22 = metadata !{i32 524324, metadata !1, metadata !"int", metadata !1, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ]
!23 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_append_bool", metadata !"tanger_stm_report_append_bool", metadata !"tanger_stm_report_append_bool", metadata !1, i32 30, metadata !20, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*, i32)* @tanger_stm_report_append_bool} ; [ DW_TAG_subprogram ]
!24 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_append_double", metadata !"tanger_stm_report_append_double", metadata !"tanger_stm_report_append_double", metadata !1, i32 31, metadata !25, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*, double)* @tanger_stm_report_append_double} ; [ DW_TAG_subprogram ]
!25 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !26, i32 0, null} ; [ DW_TAG_subroutine_type ]
!26 = metadata !{null, metadata !5, metadata !6, metadata !27}
!27 = metadata !{i32 524324, metadata !1, metadata !"double", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ]
!28 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_append_string", metadata !"tanger_stm_report_append_string", metadata !"tanger_stm_report_append_string", metadata !1, i32 32, metadata !29, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*, i8*)* @tanger_stm_report_append_string} ; [ DW_TAG_subprogram ]
!29 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !30, i32 0, null} ; [ DW_TAG_subroutine_type ]
!30 = metadata !{null, metadata !5, metadata !6, metadata !6}
!31 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_finish_object", metadata !"tanger_stm_report_finish_object", metadata !"tanger_stm_report_finish_object", metadata !1, i32 33, metadata !32, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*)* @tanger_stm_report_finish_object} ; [ DW_TAG_subprogram ]
!32 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !33, i32 0, null} ; [ DW_TAG_subroutine_type ]
!33 = metadata !{null, metadata !5}
!34 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_finish_array", metadata !"tanger_stm_report_finish_array", metadata !"tanger_stm_report_finish_array", metadata !1, i32 34, metadata !32, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*)* @tanger_stm_report_finish_array} ; [ DW_TAG_subprogram ]
!35 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_report_finish", metadata !"tanger_stm_report_finish", metadata !"tanger_stm_report_finish", metadata !1, i32 35, metadata !36, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*)* @tanger_stm_report_finish} ; [ DW_TAG_subprogram ]
!36 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !37, i32 0, null} ; [ DW_TAG_subroutine_type ]
!37 = metadata !{null, metadata !5, metadata !6}
!38 = metadata !{i32 524545, metadata !0, metadata !"name", metadata !1, i32 25, metadata !6} ; [ DW_TAG_arg_variable ]
!39 = metadata !{i32 524545, metadata !9, metadata !"handle", metadata !1, i32 26, metadata !5} ; [ DW_TAG_arg_variable ]
!40 = metadata !{i32 524545, metadata !9, metadata !"name", metadata !1, i32 26, metadata !6} ; [ DW_TAG_arg_variable ]
!41 = metadata !{i32 524545, metadata !12, metadata !"handle", metadata !1, i32 27, metadata !5} ; [ DW_TAG_arg_variable ]
!42 = metadata !{i32 524545, metadata !12, metadata !"name", metadata !1, i32 27, metadata !6} ; [ DW_TAG_arg_variable ]
!43 = metadata !{i32 524545, metadata !13, metadata !"handle", metadata !1, i32 28, metadata !5} ; [ DW_TAG_arg_variable ]
!44 = metadata !{i32 524545, metadata !13, metadata !"name", metadata !1, i32 28, metadata !6} ; [ DW_TAG_arg_variable ]
!45 = metadata !{i32 524545, metadata !13, metadata !"i", metadata !1, i32 28, metadata !16} ; [ DW_TAG_arg_variable ]
!46 = metadata !{i32 524545, metadata !19, metadata !"handle", metadata !1, i32 29, metadata !5} ; [ DW_TAG_arg_variable ]
!47 = metadata !{i32 524545, metadata !19, metadata !"name", metadata !1, i32 29, metadata !6} ; [ DW_TAG_arg_variable ]
!48 = metadata !{i32 524545, metadata !19, metadata !"i", metadata !1, i32 29, metadata !22} ; [ DW_TAG_arg_variable ]
!49 = metadata !{i32 524545, metadata !23, metadata !"handle", metadata !1, i32 30, metadata !5} ; [ DW_TAG_arg_variable ]
!50 = metadata !{i32 524545, metadata !23, metadata !"name", metadata !1, i32 30, metadata !6} ; [ DW_TAG_arg_variable ]
!51 = metadata !{i32 524545, metadata !23, metadata !"i", metadata !1, i32 30, metadata !22} ; [ DW_TAG_arg_variable ]
!52 = metadata !{i32 524545, metadata !24, metadata !"handle", metadata !1, i32 31, metadata !5} ; [ DW_TAG_arg_variable ]
!53 = metadata !{i32 524545, metadata !24, metadata !"name", metadata !1, i32 31, metadata !6} ; [ DW_TAG_arg_variable ]
!54 = metadata !{i32 524545, metadata !24, metadata !"d", metadata !1, i32 31, metadata !27} ; [ DW_TAG_arg_variable ]
!55 = metadata !{i32 524545, metadata !28, metadata !"handle", metadata !1, i32 32, metadata !5} ; [ DW_TAG_arg_variable ]
!56 = metadata !{i32 524545, metadata !28, metadata !"name", metadata !1, i32 32, metadata !6} ; [ DW_TAG_arg_variable ]
!57 = metadata !{i32 524545, metadata !28, metadata !"str", metadata !1, i32 32, metadata !6} ; [ DW_TAG_arg_variable ]
!58 = metadata !{i32 524545, metadata !31, metadata !"handle", metadata !1, i32 33, metadata !5} ; [ DW_TAG_arg_variable ]
!59 = metadata !{i32 524545, metadata !34, metadata !"handle", metadata !1, i32 34, metadata !5} ; [ DW_TAG_arg_variable ]
!60 = metadata !{i32 524545, metadata !35, metadata !"handle", metadata !1, i32 35, metadata !5} ; [ DW_TAG_arg_variable ]
!61 = metadata !{i32 524545, metadata !35, metadata !"name", metadata !1, i32 35, metadata !6} ; [ DW_TAG_arg_variable ]
!62 = metadata !{i32 25, i32 0, metadata !0, null}
!63 = metadata !{i32 25, i32 0, metadata !64, null}
!64 = metadata !{i32 524299, metadata !0, i32 25, i32 0, metadata !1, i32 0} ; [ DW_TAG_lexical_block ]
!65 = metadata !{i32 26, i32 0, metadata !9, null}
!66 = metadata !{i32 26, i32 0, metadata !67, null}
!67 = metadata !{i32 524299, metadata !9, i32 26, i32 0, metadata !1, i32 1} ; [ DW_TAG_lexical_block ]
!68 = metadata !{i32 27, i32 0, metadata !12, null}
!69 = metadata !{i32 27, i32 0, metadata !70, null}
!70 = metadata !{i32 524299, metadata !12, i32 27, i32 0, metadata !1, i32 2} ; [ DW_TAG_lexical_block ]
!71 = metadata !{i32 28, i32 0, metadata !13, null}
!72 = metadata !{i32 28, i32 0, metadata !73, null}
!73 = metadata !{i32 524299, metadata !13, i32 28, i32 0, metadata !1, i32 3} ; [ DW_TAG_lexical_block ]
!74 = metadata !{i32 29, i32 0, metadata !19, null}
!75 = metadata !{i32 29, i32 0, metadata !76, null}
!76 = metadata !{i32 524299, metadata !19, i32 29, i32 0, metadata !1, i32 4} ; [ DW_TAG_lexical_block ]
!77 = metadata !{i32 30, i32 0, metadata !23, null}
!78 = metadata !{i32 30, i32 0, metadata !79, null}
!79 = metadata !{i32 524299, metadata !23, i32 30, i32 0, metadata !1, i32 5} ; [ DW_TAG_lexical_block ]
!80 = metadata !{i32 31, i32 0, metadata !24, null}
!81 = metadata !{i32 31, i32 0, metadata !82, null}
!82 = metadata !{i32 524299, metadata !24, i32 31, i32 0, metadata !1, i32 6} ; [ DW_TAG_lexical_block ]
!83 = metadata !{i32 32, i32 0, metadata !28, null}
!84 = metadata !{i32 32, i32 0, metadata !85, null}
!85 = metadata !{i32 524299, metadata !28, i32 32, i32 0, metadata !1, i32 7} ; [ DW_TAG_lexical_block ]
!86 = metadata !{i32 33, i32 0, metadata !31, null}
!87 = metadata !{i32 33, i32 0, metadata !88, null}
!88 = metadata !{i32 524299, metadata !31, i32 33, i32 0, metadata !1, i32 8} ; [ DW_TAG_lexical_block ]
!89 = metadata !{i32 34, i32 0, metadata !34, null}
!90 = metadata !{i32 34, i32 0, metadata !91, null}
!91 = metadata !{i32 524299, metadata !34, i32 34, i32 0, metadata !1, i32 9} ; [ DW_TAG_lexical_block ]
!92 = metadata !{i32 35, i32 0, metadata !35, null}
!93 = metadata !{i32 35, i32 0, metadata !94, null}
!94 = metadata !{i32 524299, metadata !35, i32 35, i32 0, metadata !1, i32 10} ; [ DW_TAG_lexical_block ]
