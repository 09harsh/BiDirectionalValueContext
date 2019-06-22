; ModuleID = 'test.ll'
source_filename = "test.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

$_Z12isPointingToIiPiEvRT_RT0_ = comdat any

$_Z12isPointingToIPPiS0_EvRT_RT0_ = comdat any

$_Z6isLiveIPPiEvRT_ = comdat any

$_Z6isLiveIPiEvRT_ = comdat any

@y = dso_local global i32** null, align 8
@v = dso_local global i32* null, align 8
@z = dso_local global i32** null, align 8
@u = dso_local global i32* null, align 8
@x = dso_local global i32* null, align 8
@w = dso_local global i32 0, align 4
@t = dso_local global i32 0, align 4

; Function Attrs: noinline norecurse optnone uwtable
define dso_local i32 @main() #0 {
bb:
  %tmp = alloca i32, align 4
  store i32 0, i32* %tmp, align 4
  store i32** @v, i32*** @y, align 8
  store i32** @u, i32*** @z, align 8
  store i32* @t, i32** @x, align 8
  call void @_Z12isPointingToIiPiEvRT_RT0_(i32* dereferenceable(4) @t, i32** dereferenceable(8) @x)
  call void @_Z12isPointingToIPPiS0_EvRT_RT0_(i32*** dereferenceable(8) @y, i32** dereferenceable(8) @v)
  %tmp1 = load i32*, i32** @x, align 8
  %tmp2 = load i32, i32* %tmp1, align 4
  %tmp3 = load i32, i32* @w, align 4
  %tmp4 = icmp sgt i32 %tmp2, %tmp3
  br i1 %tmp4, label %bb5, label %bb6

bb5:                                              ; preds = %bb
  store i32* @w, i32** @x, align 8
  br label %bb9

bb6:                                              ; preds = %bb
  %tmp7 = load i32**, i32*** @y, align 8
  %tmp8 = load i32*, i32** %tmp7, align 8
  store i32* %tmp8, i32** @x, align 8
  br label %bb9

bb9:                                              ; preds = %bb6, %bb5
  br label %bb10

bb10:                                             ; preds = %bb13, %bb9
  %tmp11 = load i32, i32* @w, align 4
  %tmp12 = icmp sgt i32 %tmp11, 0
  br i1 %tmp12, label %bb13, label %bb16

bb13:                                             ; preds = %bb10
  call void @_Z6isLiveIPPiEvRT_(i32*** dereferenceable(8) @y)
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @u)
  call void @_Z6isLiveIPPiEvRT_(i32*** dereferenceable(8) @z)
  call void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) @x)
  %tmp14 = load i32**, i32*** @y, align 8
  %tmp15 = load i32*, i32** %tmp14, align 8
  store i32* %tmp15, i32** @x, align 8
  store i32** @v, i32*** @z, align 8
  br label %bb10

bb16:                                             ; preds = %bb10
  %tmp17 = load i32**, i32*** @z, align 8
  %tmp18 = load i32*, i32** %tmp17, align 8
  %tmp19 = load i32, i32* %tmp18, align 4
  ret i32 %tmp19
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_Z12isPointingToIiPiEvRT_RT0_(i32* dereferenceable(4) %arg, i32** dereferenceable(8) %arg1) #1 comdat {
bb:
  %tmp = alloca i32*, align 8
  %tmp2 = alloca i32**, align 8
  store i32* %arg, i32** %tmp, align 8
  store i32** %arg1, i32*** %tmp2, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_Z12isPointingToIPPiS0_EvRT_RT0_(i32*** dereferenceable(8) %arg, i32** dereferenceable(8) %arg1) #1 comdat {
bb:
  %tmp = alloca i32***, align 8
  %tmp2 = alloca i32**, align 8
  store i32*** %arg, i32**** %tmp, align 8
  store i32** %arg1, i32*** %tmp2, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_Z6isLiveIPPiEvRT_(i32*** dereferenceable(8) %arg) #1 comdat {
bb:
  %tmp = alloca i32***, align 8
  store i32*** %arg, i32**** %tmp, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_Z6isLiveIPiEvRT_(i32** dereferenceable(8) %arg) #1 comdat {
bb:
  %tmp = alloca i32**, align 8
  store i32** %arg, i32*** %tmp, align 8
  ret void
}

attributes #0 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0-3 (tags/RELEASE_800/final)"}
