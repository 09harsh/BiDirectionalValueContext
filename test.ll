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
  ret i32 %tmp3
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
  %tmp21 = load i32, i32* %tmp5, align 4
  %tmp22 = load i32, i32* %tmp6, align 4
  %tmp23 = sub nsw i32 %tmp21, %tmp22
  store i32 %tmp23, i32* %tmp2, align 4
  %tmp24 = load i32, i32* %tmp4, align 4
  %tmp25 = load i32, i32* %tmp6, align 4
  %tmp26 = mul nsw i32 %tmp24, %tmp25
  %tmp27 = load i32, i32* %tmp5, align 4
  %tmp28 = sdiv i32 %tmp26, %tmp27
  store i32 %tmp28, i32* %tmp4, align 4
  br label %bb29

bb29:                                             ; preds = %bb32, %bb
  %tmp30 = load i32, i32* %tmp5, align 4
  %tmp31 = icmp sgt i32 %tmp30, 3
  br i1 %tmp31, label %bb32, label %bb38

bb32:                                             ; preds = %bb29
  %tmp33 = load i32, i32* %tmp4, align 4
  %tmp34 = load i32, i32* %tmp3, align 4
  %tmp35 = mul nsw i32 %tmp33, %tmp34
  %tmp36 = load i32, i32* %tmp2, align 4
  %tmp37 = add nsw i32 %tmp35, %tmp36
  store i32 %tmp37, i32* %tmp4, align 4
  br label %bb29

bb38:                                             ; preds = %bb29
  %tmp39 = load i32, i32* %tmp1, align 4
  %tmp40 = icmp sgt i32 4, %tmp39
  br i1 %tmp40, label %bb41, label %bb45

bb41:                                             ; preds = %bb38
  %tmp42 = load i32, i32* %tmp1, align 4
  %tmp43 = load i32, i32* %tmp2, align 4
  %tmp44 = mul nsw i32 %tmp42, %tmp43
  store i32 %tmp44, i32* %tmp5, align 4
  br label %bb49

bb45:                                             ; preds = %bb38
  %tmp46 = load i32, i32* %tmp3, align 4
  %tmp47 = load i32, i32* %tmp6, align 4
  %tmp48 = sub nsw i32 %tmp46, %tmp47
  store i32 %tmp48, i32* %tmp6, align 4
  br label %bb49

bb49:                                             ; preds = %bb45, %bb41
  %tmp50 = load i32, i32* %tmp1, align 4
  %tmp51 = load i32, i32* %tmp2, align 4
  %tmp52 = call i32 @_Z4funcii(i32 %tmp50, i32 %tmp51)
  store i32 %tmp52, i32* %tmp3, align 4
  %tmp53 = load i32, i32* %tmp3, align 4
  ret i32 %tmp53
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline norecurse nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0-3 (tags/RELEASE_800/final)"}
