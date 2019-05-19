; ModuleID = 'test.ll'
source_filename = "test.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5checkv() #0 {
bb:
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_Z4funcii(i32 %arg, i32 %arg1) #0 {
bb:
  %tmp = alloca i32, align 4
  %tmp2 = alloca i32, align 4
  store i32 %arg, i32* %tmp, align 4
  store i32 %arg1, i32* %tmp2, align 4
  call void @_Z5checkv()
  %tmp3 = load i32, i32* %tmp, align 4
  %tmp4 = load i32, i32* %tmp2, align 4
  %tmp5 = add nsw i32 %tmp3, %tmp4
  store i32 %tmp5, i32* %tmp, align 4
  %tmp6 = load i32, i32* %tmp, align 4
  ret i32 %tmp6
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z8functionv() #0 {
bb:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i32, align 4
  call void @_Z5checkv()
  store i32 8, i32* %tmp, align 4
  store i32 7, i32* %tmp1, align 4
  ret void
}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dso_local i32 @main() #1 {
bb:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i32, align 4
  %tmp2 = alloca i32, align 4
  %tmp3 = alloca i32, align 4
  %tmp4 = alloca i32, align 4
  %tmp5 = alloca i32, align 4
  %tmp6 = alloca i32, align 4
  store i32 0, i32* %tmp, align 4
  call void @_Z5checkv()
  %tmp7 = load i32, i32* %tmp1, align 4
  %tmp8 = load i32, i32* %tmp2, align 4
  %tmp9 = add nsw i32 %tmp7, %tmp8
  %tmp10 = load i32, i32* %tmp3, align 4
  %tmp11 = load i32, i32* %tmp4, align 4
  %tmp12 = mul nsw i32 %tmp10, %tmp11
  %tmp13 = load i32, i32* %tmp5, align 4
  %tmp14 = sdiv i32 %tmp12, %tmp13
  %tmp15 = load i32, i32* %tmp6, align 4
  %tmp16 = srem i32 %tmp14, %tmp15
  %tmp17 = sub nsw i32 %tmp9, %tmp16
  store i32 %tmp17, i32* %tmp1, align 4
  %tmp18 = load i32, i32* %tmp2, align 4
  %tmp19 = load i32, i32* %tmp4, align 4
  %tmp20 = add nsw i32 %tmp18, %tmp19
  store i32 %tmp20, i32* %tmp1, align 4
  %tmp21 = load i32, i32* %tmp1, align 4
  %tmp22 = load i32, i32* %tmp2, align 4
  %tmp23 = icmp sgt i32 %tmp21, %tmp22
  br i1 %tmp23, label %bb24, label %bb26

bb24:                                             ; preds = %bb
  %tmp25 = load i32, i32* %tmp3, align 4
  store i32 %tmp25, i32* %tmp1, align 4
  br label %bb26

bb26:                                             ; preds = %bb24, %bb
  %tmp27 = load i32, i32* %tmp1, align 4
  %tmp28 = load i32, i32* %tmp2, align 4
  %tmp29 = call i32 @_Z4funcii(i32 %tmp27, i32 %tmp28)
  store i32 %tmp29, i32* %tmp3, align 4
  %tmp30 = load i32, i32* %tmp5, align 4
  %tmp31 = load i32, i32* %tmp6, align 4
  %tmp32 = sub nsw i32 %tmp30, %tmp31
  store i32 %tmp32, i32* %tmp2, align 4
  %tmp33 = load i32, i32* %tmp4, align 4
  %tmp34 = load i32, i32* %tmp6, align 4
  %tmp35 = mul nsw i32 %tmp33, %tmp34
  %tmp36 = load i32, i32* %tmp5, align 4
  %tmp37 = sdiv i32 %tmp35, %tmp36
  store i32 %tmp37, i32* %tmp4, align 4
  br label %bb38

bb38:                                             ; preds = %bb41, %bb26
  %tmp39 = load i32, i32* %tmp5, align 4
  %tmp40 = icmp sgt i32 %tmp39, 3
  br i1 %tmp40, label %bb41, label %bb47

bb41:                                             ; preds = %bb38
  %tmp42 = load i32, i32* %tmp4, align 4
  %tmp43 = load i32, i32* %tmp3, align 4
  %tmp44 = mul nsw i32 %tmp42, %tmp43
  %tmp45 = load i32, i32* %tmp2, align 4
  %tmp46 = add nsw i32 %tmp44, %tmp45
  store i32 %tmp46, i32* %tmp4, align 4
  br label %bb38

bb47:                                             ; preds = %bb38
  call void @_Z8functionv()
  %tmp48 = load i32, i32* %tmp1, align 4
  %tmp49 = icmp sgt i32 4, %tmp48
  br i1 %tmp49, label %bb50, label %bb54

bb50:                                             ; preds = %bb47
  %tmp51 = load i32, i32* %tmp1, align 4
  %tmp52 = load i32, i32* %tmp2, align 4
  %tmp53 = mul nsw i32 %tmp51, %tmp52
  store i32 %tmp53, i32* %tmp5, align 4
  br label %bb58

bb54:                                             ; preds = %bb47
  %tmp55 = load i32, i32* %tmp3, align 4
  %tmp56 = load i32, i32* %tmp6, align 4
  %tmp57 = sub nsw i32 %tmp55, %tmp56
  store i32 %tmp57, i32* %tmp6, align 4
  br label %bb58

bb58:                                             ; preds = %bb54, %bb50
  %tmp59 = load i32, i32* %tmp3, align 4
  ret i32 %tmp59
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline norecurse nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0-3 (tags/RELEASE_800/final)"}
