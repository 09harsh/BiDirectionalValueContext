; ModuleID = 'test.ll'
source_filename = "test.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

$_Z6isLiveIPiEvRT_ = comdat any

$_Z12isPointingToIPiiEvRT_RT0_ = comdat any

@y = dso_local global i32** null, align 8
@v = dso_local global i32* null, align 8
@z = dso_local global i32** null, align 8
@u = dso_local global i32* null, align 8
@x = dso_local global i32* null, align 8
@w = dso_local global i32 0, align 4
@b = dso_local global i32 0, align 4

; Function Attrs: noinline optnone uwtable
define dso_local void @_Z4funcv() #0 {
bb:
  %tmp = load i32*, i32** @u, align 8
  store i32* %tmp, i32** @x, align 8
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @v)
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
  store i32** @v, i32*** @y, align 8
  store i32** @u, i32*** @z, align 8
  store i32* @w, i32** @x, align 8
  store i32* @b, i32** @v, align 8
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @u)
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @x)
  br label %bb1

bb1:                                              ; preds = %bb4, %bb
  %tmp2 = load i32, i32* @w, align 4
  %tmp3 = icmp sgt i32 %tmp2, 0
  br i1 %tmp3, label %bb4, label %bb7

bb4:                                              ; preds = %bb1
  call void @_Z4funcv()
  %tmp5 = load i32**, i32*** @y, align 8
  %tmp6 = load i32*, i32** %tmp5, align 8
  store i32* %tmp6, i32** @u, align 8
  br label %bb1

bb7:                                              ; preds = %bb1
  call void @_Z12isPointingToIPiiEvRT_RT0_(i32** dereferenceable(8) @x, i32* dereferenceable(4) @b)
  %tmp8 = load i32*, i32** @x, align 8
  %tmp9 = load i32, i32* %tmp8, align 4
  ret i32 %tmp9
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_Z12isPointingToIPiiEvRT_RT0_(i32** dereferenceable(8) %arg, i32* dereferenceable(4) %arg1) #1 comdat {
bb:
  %tmp = alloca i32**, align 8
  %tmp2 = alloca i32*, align 8
  store i32** %arg, i32*** %tmp, align 8
  store i32* %arg1, i32** %tmp2, align 8
  ret void
}

attributes #0 = { noinline optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0-3 (tags/RELEASE_800/final)"}
