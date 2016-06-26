; ModuleID = 'indirectcalls.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@tanger_stm_indirect_target_pairs_multi = common global i8** null
@tanger_stm_indirect_nb_versions = common global i32 0
@tanger_stm_indirect_nb_targets_multi = common global i32 0
@tanger_stm_indirect_nb_targets_max_multi = common global i32 0

define i8* @tanger_stm_indirect_resolve_multiple(i8* %nontxnal_function, i32 %version) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %nontxnal_function}, i64 0, metadata !22), !dbg !32
  tail call void @llvm.dbg.value(metadata !{i32 %version}, i64 0, metadata !23), !dbg !32
  %0 = load i32* @tanger_stm_indirect_nb_targets_multi, align 4, !dbg !33
  %1 = load i32* @tanger_stm_indirect_nb_versions, align 4, !dbg !33
  %2 = mul i32 %1, %0, !dbg !33
  tail call void @llvm.dbg.value(metadata !{i32 %2}, i64 0, metadata !26), !dbg !33
  tail call void @llvm.dbg.value(metadata !34, i64 0, metadata !24), !dbg !35
  %3 = load i8*** @tanger_stm_indirect_target_pairs_multi, align 8, !dbg !36
  br label %bb3, !dbg !35

bb:                                               ; preds = %bb3
  %4 = zext i32 %i.0 to i64, !dbg !36
  %5 = getelementptr inbounds i8** %3, i64 %4, !dbg !36
  %6 = load i8** %5, align 8, !dbg !36
  %7 = icmp eq i8* %6, %nontxnal_function, !dbg !36
  br i1 %7, label %bb1, label %bb2, !dbg !36

bb1:                                              ; preds = %bb
  %8 = add i32 %version, 1, !dbg !37
  %9 = add i32 %8, %i.0, !dbg !37
  %10 = zext i32 %9 to i64, !dbg !37
  %11 = getelementptr inbounds i8** %3, i64 %10, !dbg !37
  %12 = load i8** %11, align 8, !dbg !37
  ret i8* %12, !dbg !37

bb2:                                              ; preds = %bb
  %indvar.next = add i32 %indvar, 1
  br label %bb3, !dbg !35

bb3:                                              ; preds = %bb2, %entry
  %indvar = phi i32 [ %indvar.next, %bb2 ], [ 0, %entry ]
  %i.0 = mul i32 %1, %indvar
  %13 = icmp ult i32 %i.0, %2, !dbg !35
  br i1 %13, label %bb, label %bb4, !dbg !35

bb4:                                              ; preds = %bb3
  tail call void @_ITM_changeTransactionMode(i32 0) nounwind, !dbg !38
  ret i8* %nontxnal_function, !dbg !37
}

define void @tanger_stm_indirect_register_multiple(i8* %nontxnal, i8* %txnal, i32 %version) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i8* %nontxnal}, i64 0, metadata !15), !dbg !39
  tail call void @llvm.dbg.value(metadata !{i8* %txnal}, i64 0, metadata !16), !dbg !39
  tail call void @llvm.dbg.value(metadata !{i32 %version}, i64 0, metadata !17), !dbg !39
  tail call void @llvm.dbg.value(metadata !34, i64 0, metadata !18), !dbg !40
  %0 = load i32* @tanger_stm_indirect_nb_targets_multi, align 4, !dbg !40
  %1 = load i8*** @tanger_stm_indirect_target_pairs_multi, align 8, !dbg !41
  %2 = load i32* @tanger_stm_indirect_nb_versions, align 4, !dbg !41
  br label %bb3, !dbg !40

bb:                                               ; preds = %bb3
  %tmp = mul i32 %2, %12
  %3 = zext i32 %tmp to i64, !dbg !41
  %4 = getelementptr inbounds i8** %1, i64 %3, !dbg !41
  %5 = load i8** %4, align 8, !dbg !41
  %6 = icmp eq i8* %5, %nontxnal, !dbg !41
  br i1 %6, label %bb1, label %bb2, !dbg !41

bb1:                                              ; preds = %bb
  %7 = add i32 %version, 1, !dbg !42
  %8 = add i32 %7, %tmp, !dbg !42
  %9 = zext i32 %8 to i64, !dbg !42
  %10 = getelementptr inbounds i8** %1, i64 %9, !dbg !42
  store i8* %txnal, i8** %10, align 8, !dbg !42
  ret void, !dbg !43

bb2:                                              ; preds = %bb
  %11 = add i32 %12, 1, !dbg !40
  br label %bb3, !dbg !40

bb3:                                              ; preds = %bb2, %entry
  %12 = phi i32 [ 0, %entry ], [ %11, %bb2 ]
  %13 = icmp ult i32 %12, %0, !dbg !40
  br i1 %13, label %bb, label %bb4, !dbg !40

bb4:                                              ; preds = %bb3
  %14 = mul i32 %2, %0, !dbg !44
  %15 = zext i32 %14 to i64, !dbg !44
  %16 = getelementptr inbounds i8** %1, i64 %15, !dbg !44
  store i8* %nontxnal, i8** %16, align 8, !dbg !44
  %17 = load i8*** @tanger_stm_indirect_target_pairs_multi, align 8, !dbg !45
  %18 = add i32 %version, 1, !dbg !45
  %19 = add i32 %18, %14, !dbg !45
  %20 = zext i32 %19 to i64, !dbg !45
  %21 = getelementptr inbounds i8** %17, i64 %20, !dbg !45
  store i8* %txnal, i8** %21, align 8, !dbg !45
  %22 = add i32 %0, 1, !dbg !46
  store i32 %22, i32* @tanger_stm_indirect_nb_targets_multi, align 4, !dbg !46
  ret void, !dbg !43
}

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

define void @tanger_stm_indirect_init_multiple(i32 %number_of_call_targets, i32 %versions) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %number_of_call_targets}, i64 0, metadata !20), !dbg !47
  tail call void @llvm.dbg.value(metadata !{i32 %versions}, i64 0, metadata !21), !dbg !47
  store i32 %number_of_call_targets, i32* @tanger_stm_indirect_nb_targets_max_multi, align 4, !dbg !48
  store i32 0, i32* @tanger_stm_indirect_nb_targets_multi, align 4, !dbg !50
  %0 = add i32 %versions, 1, !dbg !51
  %1 = mul i32 %0, %number_of_call_targets, !dbg !51
  %2 = zext i32 %1 to i64, !dbg !51
  %3 = tail call noalias i8* @calloc(i64 %2, i64 8) nounwind, !dbg !51
  %4 = bitcast i8* %3 to i8**, !dbg !51
  store i8** %4, i8*** @tanger_stm_indirect_target_pairs_multi, align 8, !dbg !51
  store i32 %0, i32* @tanger_stm_indirect_nb_versions, align 4, !dbg !52
  ret void, !dbg !53
}

declare noalias i8* @calloc(i64, i64) nounwind

declare void @_ITM_changeTransactionMode(i32)

!llvm.dbg.sp = !{!0, !9, !12}
!llvm.dbg.lv.tanger_stm_indirect_register_multiple = !{!15, !16, !17, !18}
!llvm.dbg.lv.tanger_stm_indirect_init_multiple = !{!20, !21}
!llvm.dbg.lv.tanger_stm_indirect_resolve_multiple = !{!22, !23, !24, !26}
!llvm.dbg.gv = !{!27, !28, !29, !30}

!0 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_indirect_register_multiple", metadata !"tanger_stm_indirect_register_multiple", metadata !"tanger_stm_indirect_register_multiple", metadata !1, i32 61, metadata !3, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i8*, i8*, i32)* @tanger_stm_indirect_register_multiple} ; [ DW_TAG_subprogram ]
!1 = metadata !{i32 524329, metadata !"indirectcalls.c", metadata !"/opt/tanger/37936/src/lib/stmsupport/", metadata !2} ; [ DW_TAG_file_type ]
!2 = metadata !{i32 524305, i32 0, i32 1, metadata !"indirectcalls.c", metadata !"/opt/tanger/37936/src/lib/stmsupport/", metadata !"4.2.1 (Based on Apple Inc. build 5658) (LLVM build)", i1 true, i1 true, metadata !"", i32 0} ; [ DW_TAG_compile_unit ]
!3 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !4, i32 0, null} ; [ DW_TAG_subroutine_type ]
!4 = metadata !{null, metadata !5, metadata !5, metadata !6}
!5 = metadata !{i32 524303, metadata !1, metadata !"", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, null} ; [ DW_TAG_pointer_type ]
!6 = metadata !{i32 524310, metadata !7, metadata !"uint32_t", metadata !7, i32 56, i64 0, i64 0, i64 0, i32 0, metadata !8} ; [ DW_TAG_typedef ]
!7 = metadata !{i32 524329, metadata !"stdint.h", metadata !"/usr/include", metadata !2} ; [ DW_TAG_file_type ]
!8 = metadata !{i32 524324, metadata !1, metadata !"unsigned int", metadata !1, i32 0, i64 32, i64 32, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ]
!9 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_indirect_init_multiple", metadata !"tanger_stm_indirect_init_multiple", metadata !"tanger_stm_indirect_init_multiple", metadata !1, i32 49, metadata !10, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, void (i32, i32)* @tanger_stm_indirect_init_multiple} ; [ DW_TAG_subprogram ]
!10 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !11, i32 0, null} ; [ DW_TAG_subroutine_type ]
!11 = metadata !{null, metadata !6, metadata !6}
!12 = metadata !{i32 524334, i32 0, metadata !1, metadata !"tanger_stm_indirect_resolve_multiple", metadata !"tanger_stm_indirect_resolve_multiple", metadata !"tanger_stm_indirect_resolve_multiple", metadata !1, i32 30, metadata !13, i1 false, i1 true, i32 0, i32 0, null, i1 false, i1 true, i8* (i8*, i32)* @tanger_stm_indirect_resolve_multiple} ; [ DW_TAG_subprogram ]
!13 = metadata !{i32 524309, metadata !1, metadata !"", metadata !1, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !14, i32 0, null} ; [ DW_TAG_subroutine_type ]
!14 = metadata !{metadata !5, metadata !5, metadata !6}
!15 = metadata !{i32 524545, metadata !0, metadata !"nontxnal", metadata !1, i32 60, metadata !5} ; [ DW_TAG_arg_variable ]
!16 = metadata !{i32 524545, metadata !0, metadata !"txnal", metadata !1, i32 60, metadata !5} ; [ DW_TAG_arg_variable ]
!17 = metadata !{i32 524545, metadata !0, metadata !"version", metadata !1, i32 60, metadata !6} ; [ DW_TAG_arg_variable ]
!18 = metadata !{i32 524544, metadata !19, metadata !"i", metadata !1, i32 62, metadata !6} ; [ DW_TAG_auto_variable ]
!19 = metadata !{i32 524299, metadata !0, i32 61, i32 0, metadata !1, i32 0} ; [ DW_TAG_lexical_block ]
!20 = metadata !{i32 524545, metadata !9, metadata !"number_of_call_targets", metadata !1, i32 48, metadata !6} ; [ DW_TAG_arg_variable ]
!21 = metadata !{i32 524545, metadata !9, metadata !"versions", metadata !1, i32 48, metadata !6} ; [ DW_TAG_arg_variable ]
!22 = metadata !{i32 524545, metadata !12, metadata !"nontxnal_function", metadata !1, i32 29, metadata !5} ; [ DW_TAG_arg_variable ]
!23 = metadata !{i32 524545, metadata !12, metadata !"version", metadata !1, i32 29, metadata !6} ; [ DW_TAG_arg_variable ]
!24 = metadata !{i32 524544, metadata !25, metadata !"i", metadata !1, i32 31, metadata !6} ; [ DW_TAG_auto_variable ]
!25 = metadata !{i32 524299, metadata !12, i32 30, i32 0, metadata !1, i32 2} ; [ DW_TAG_lexical_block ]
!26 = metadata !{i32 524544, metadata !25, metadata !"max_nr", metadata !1, i32 32, metadata !6} ; [ DW_TAG_auto_variable ]
!27 = metadata !{i32 524340, i32 0, metadata !1, metadata !"tanger_stm_indirect_nb_targets_max_multi", metadata !"tanger_stm_indirect_nb_targets_max_multi", metadata !"", metadata !1, i32 17, metadata !6, i1 false, i1 true, i32* @tanger_stm_indirect_nb_targets_max_multi} ; [ DW_TAG_variable ]
!28 = metadata !{i32 524340, i32 0, metadata !1, metadata !"tanger_stm_indirect_nb_targets_multi", metadata !"tanger_stm_indirect_nb_targets_multi", metadata !"", metadata !1, i32 18, metadata !6, i1 false, i1 true, i32* @tanger_stm_indirect_nb_targets_multi} ; [ DW_TAG_variable ]
!29 = metadata !{i32 524340, i32 0, metadata !1, metadata !"tanger_stm_indirect_nb_versions", metadata !"tanger_stm_indirect_nb_versions", metadata !"", metadata !1, i32 20, metadata !6, i1 false, i1 true, i32* @tanger_stm_indirect_nb_versions} ; [ DW_TAG_variable ]
!30 = metadata !{i32 524340, i32 0, metadata !1, metadata !"tanger_stm_indirect_target_pairs_multi", metadata !"tanger_stm_indirect_target_pairs_multi", metadata !"", metadata !1, i32 22, metadata !31, i1 false, i1 true, i8*** @tanger_stm_indirect_target_pairs_multi} ; [ DW_TAG_variable ]
!31 = metadata !{i32 524303, metadata !1, metadata !"", metadata !1, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !5} ; [ DW_TAG_pointer_type ]
!32 = metadata !{i32 29, i32 0, metadata !12, null}
!33 = metadata !{i32 32, i32 0, metadata !25, null}
!34 = metadata !{i32 0}
!35 = metadata !{i32 33, i32 0, metadata !25, null}
!36 = metadata !{i32 34, i32 0, metadata !25, null}
!37 = metadata !{i32 35, i32 0, metadata !25, null}
!38 = metadata !{i32 39, i32 0, metadata !25, null}
!39 = metadata !{i32 60, i32 0, metadata !0, null}
!40 = metadata !{i32 64, i32 0, metadata !19, null}
!41 = metadata !{i32 66, i32 0, metadata !19, null}
!42 = metadata !{i32 68, i32 0, metadata !19, null}
!43 = metadata !{i32 69, i32 0, metadata !19, null}
!44 = metadata !{i32 74, i32 0, metadata !19, null}
!45 = metadata !{i32 76, i32 0, metadata !19, null}
!46 = metadata !{i32 78, i32 0, metadata !19, null}
!47 = metadata !{i32 48, i32 0, metadata !9, null}
!48 = metadata !{i32 50, i32 0, metadata !49, null}
!49 = metadata !{i32 524299, metadata !9, i32 49, i32 0, metadata !1, i32 1} ; [ DW_TAG_lexical_block ]
!50 = metadata !{i32 51, i32 0, metadata !49, null}
!51 = metadata !{i32 53, i32 0, metadata !49, null}
!52 = metadata !{i32 54, i32 0, metadata !49, null}
!53 = metadata !{i32 55, i32 0, metadata !49, null}
