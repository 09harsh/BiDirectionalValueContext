; ModuleID = 'test.ll'
source_filename = "test.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

$_Z6isLiveIPiEvRT_ = comdat any

@a = dso_local global i32* null, align 8
@b = dso_local global i32 0, align 4
@c = dso_local global i32* null, align 8
@d = dso_local global i32* null, align 8

; Function Attrs: noinline optnone uwtable
define dso_local void @_Z4funci(i32 %arg) #0 {
bb:
  %tmp = alloca i32, align 4
  store i32 %arg, i32* %tmp, align 4
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @a)
  %tmp1 = load i32*, i32** @d, align 8
  store i32* %tmp1, i32** @c, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) %arg) #1 comdat {
bb:
  %tmp = alloca i32**, align 8
  store i32** %arg, i32*** %tmp, align 8
  ret void
}

; Function Attrs: noinline norecurse optnone uwtable
define dso_local i32 @main() #2 {
bb:
  %tmp = alloca i32, align 4
  store i32 0, i32* %tmp, align 4
  %tmp1 = load i32*, i32** @d, align 8
  store i32* %tmp1, i32** @a, align 8
  %tmp2 = load i32, i32* @b, align 4
  call void @_Z4funci(i32 %tmp2)
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @c)
  %tmp3 = load i32*, i32** @a, align 8
  store i32* %tmp3, i32** @d, align 8
  %tmp4 = load i32*, i32** @c, align 8
  %tmp5 = load i32, i32* %tmp4, align 4
  call void @_Z4funci(i32 %tmp5)
  %tmp6 = load i32*, i32** @c, align 8
  %tmp7 = load i32, i32* %tmp6, align 4
  ret i32 %tmp7
}

attributes #0 = { noinline optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0-3 (tags/RELEASE_800/final)"}
