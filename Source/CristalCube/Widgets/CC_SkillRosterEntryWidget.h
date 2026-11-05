// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_UISurface.h"
#include "CC_SkillRosterEntryWidget.generated.h"

class UCC_SkillBase;
class ACC_PlayerState;
class UCC_UIIconWell;
class UTextBlock;
class USizeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRosterEntrySelectRequested, class UCC_SkillRosterEntryWidget*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRosterSlotDropped, int32, SourceSlotIndex, int32, TargetSlotIndex);

/**
 * WBP_SkillRosterEntry 구조 (이름 일치):
 *   [Root] EntrySizeBox (Size Box, 선택 — 높이 76 = Token EntryHeight)
 *     └ Overlay
 *        ├ EntrySurface (WBP_UI_Surface, Fill/Fill)                  BindWidget — C++: Card · Entry 18
 *        └ Horizontal Box (Padding 16,0 · 세로 Center)
 *           ├ SkillWell (WBP_UI_IconWell — Well Size M)               BindWidget
 *           ├ Spacer 14×1
 *           └ Vertical Box (Fill · 세로 Center)
 *              ├ Horizontal Box : SkillName (TextBlock) · Spacer 8 · NoticeDot (WBP_UI_NoticeDot, 선택, 세로 Center)
 *              ├ Spacer 1×4
 *              └ SummaryText (TextBlock, 선택)
 *        (EmptySlotOverlay — 선택, 빈 슬롯일 때만 보임: 예 "+" 글리프)
 *   클릭·드래그는 이 위젯 자체가 받음 (자식은 전부 Not Hit-Testable — C++가 지정)
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillRosterEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 슬롯 데이터 — Skill nullptr = 빈 슬롯 (드롭 대상 전용). 요약 · Notice Dot도 여기서 다시 계산 */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void SetSkillData(int32 InSlotIndex, UCC_SkillBase* Skill, ACC_PlayerState* PlayerState);

	/** 부모(인벤토리)가 지시 — 선택 카드 = Surface Selected */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void SetSelected(bool bInSelected, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	bool IsSelected() const { return bSelected; }

	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	UCC_SkillBase* GetBoundSkill() const { return BoundSkill; }

	/** Core 은행 > 0 또는 이 스킬이 가진 Addon 은행 > 0 */
	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	bool HasUnspentPoints() const { return bHasUnspent; }

	UPROPERTY(BlueprintAssignable, Category = "Skill Roster")
	FOnRosterEntrySelectRequested OnSelectRequested;

	/** Ctrl+드래그로 슬롯이 재배치되었을 때 — 실제 SwapSlots 호출은 부모 */
	UPROPERTY(BlueprintAssignable, Category = "Skill Roster")
	FOnRosterSlotDropped OnSlotDropRequested;

	/** (fix1) 드래그 복제본 전용 — 입력 안 받음 · 폭 고정 · Selected 모양 · 반투명 */
	void PrepareAsDragGhost(float Width);

	/** (fix1) 드래그 중 원래 자리 카드 투명도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Roster|Drag", meta = (ClampMin = "0", ClampMax = "1"))
	float DragSourceOpacity = 0.35f;

	/** (fix1) 커서를 따라오는 복제본 투명도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Roster|Drag", meta = (ClampMin = "0", ClampMax = "1"))
	float DragGhostOpacity = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Roster")
	FText EmptyName = NSLOCTEXT("CC_UI", "RosterEmptyName", "빈 슬롯");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Roster")
	FText EmptySummary = NSLOCTEXT("CC_UI", "RosterEmptySummary", "EMPTY");

	// ── 디자이너 미리보기 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Roster|Preview")
	FText PreviewName = NSLOCTEXT("CC_UI", "RosterPreviewName", "Fireball");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Roster|Preview")
	FText PreviewSummary = NSLOCTEXT("CC_UI", "RosterPreviewSummary", "Projectile · CORE 5 · ADDON 2");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Roster|Preview")
	ESkillElementType PreviewElement = ESkillElementType::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Roster|Preview")
	bool bPreviewSelected = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Roster|Preview")
	bool bPreviewUnspent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Roster|Preview")
	bool bPreviewEmpty = false;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, class UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, class UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, class UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, class UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_UISurface> EntrySurface;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_UIIconWell> SkillWell;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NoticeDot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> EntrySizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EmptySlotOverlay;

private:
	/** (fix1) 드래그가 끝나면(놓음·취소) 원래 자리 카드 복원 — DragOp.OnDrop / OnDragCancelled */
	UFUNCTION()
	void HandleDragEnded(UDragDropOperation* Operation);

	void SetDragSource(bool bInDragSource);

	bool bDragSource = false;   // 내가 잡혀서 끌려가는 중
	bool bDragOver = false;     // 다른 카드가 내 위에 떠 있음 (놓을 자리 표시)
	bool bIsDragGhost = false;  // 커서를 따라오는 복제본

	void ApplySurfaceConfig();
	void ApplySurfaceState(bool bInstant);
	void ApplyTextStyles(bool bEmpty);
	void ApplyPreview();
	bool IsEmptySlot() const;

	int32 SlotIndex = INDEX_NONE;
	bool bSelected = false;
	bool bHovered = false;
	bool bPendingClick = false;
	bool bHasUnspent = false;
	bool bDataSet = false;
	ESkillElementType CurrentElement = ESkillElementType::None;

	UPROPERTY(Transient)
	TObjectPtr<UCC_SkillBase> BoundSkill;

	UPROPERTY(Transient)
	TObjectPtr<ACC_PlayerState> BoundPlayerState;
};