open Globals;
open LetOps;
open Mocha;
open SmockGeneral;

let testUnit =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("shiftTokens", () => {
    let marketIndex = Helpers.randomJsInteger();
    let amountSyntheticTokensToShift = Helpers.randomTokenAmount();
    let amountSyntheticTokensToShiftBeforeValue = Helpers.randomTokenAmount();

    before_once'(() => {
      let {staker, longShortSmocked} = contracts.contents;
      let%Await _ =
        staker->Staker.Exposed.setLongShort(
          ~longShort=longShortSmocked.address,
        );
      contracts.contents.staker
      ->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="shiftTokens",
        );
    });

    let setup =
        (
          ~isShiftFromLong,
          ~amountSyntheticTokensToShiftBeforeValue,
          ~amountSyntheticTokensToShift,
          ~userNextPrice_stakedSyntheticTokenShiftIndex,
          ~latestRewardIndex,
          ~userAmountStaked,
        ) => {
      let user = accounts.contents->Array.getUnsafe(0).address;
      let {staker, syntheticTokenSmocked} = contracts.contents;

      let%Await _ =
        staker->Staker.Exposed.setShiftTokensParams(
          ~marketIndex,
          ~isShiftFromLong,
          ~user,
          ~amountSyntheticTokensToShift=amountSyntheticTokensToShiftBeforeValue,
          ~userNextPrice_stakedSyntheticTokenShiftIndex,
          ~latestRewardIndex,
          ~userAmountStaked,
          ~syntheticToken=syntheticTokenSmocked.address,
        );
      staker->Staker.shiftTokens(
        ~amountSyntheticTokensToShift,
        ~marketIndex,
        ~isShiftFromLong,
      );
    };

    let isShiftFromLong = true;

    it(
      "reverts if market doesn't exist or user doesn't have any staked tokens",
      () => {
      Chai.expectRevert(
        ~transaction=
          contracts.contents.staker
          ->Staker.shiftTokens(
              ~amountSyntheticTokensToShift,
              ~marketIndex,
              ~isShiftFromLong,
            ),
        ~reason="Not enough tokens to shift",
      )
    });

    it(
      "calls _mintAccumulatedFloatAndExecuteOutstandingShifts (via modifier) with the correct arguments if the user has a 'confirmed' shift that needs to be settled",
      () => {
        let user = accounts.contents->Array.getUnsafe(0).address;
        let userNextPrice_stakedSyntheticTokenShiftIndex =
          Helpers.randomInteger();
        let latestRewardIndex =
          userNextPrice_stakedSyntheticTokenShiftIndex->add(
            Helpers.randomInteger(),
          );

        let%Await _ =
          setup(
            ~isShiftFromLong,
            ~amountSyntheticTokensToShiftBeforeValue,
            ~userNextPrice_stakedSyntheticTokenShiftIndex,
            ~latestRewardIndex,
            ~amountSyntheticTokensToShift,
            ~userAmountStaked=
              amountSyntheticTokensToShift->add(
                amountSyntheticTokensToShiftBeforeValue,
              ),
          );

        StakerSmocked.InternalMock._updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingShiftsCallCheck({
          marketIndex,
          user,
        });
      },
    );

    it(
      "doesn't call _mintAccumulatedFloatAndExecuteOutstandingShifts if userNextPrice_stakedSyntheticTokenShiftIndex == 0",
      () => {
        let latestRewardIndex = Helpers.randomInteger();

        let%Await _ =
          setup(
            ~isShiftFromLong,
            ~amountSyntheticTokensToShiftBeforeValue,
            ~userNextPrice_stakedSyntheticTokenShiftIndex=zeroBn,
            ~latestRewardIndex,
            ~amountSyntheticTokensToShift,
            ~userAmountStaked=
              amountSyntheticTokensToShift->add(
                amountSyntheticTokensToShiftBeforeValue,
              ),
          );
        ();
        expect(
          StakerSmocked.InternalMock._mintAccumulatedFloatAndExecuteOutstandingShiftsFunction(),
        )
        ->toHaveCallCount(0);
      },
    );
    it(
      "doesn't call _mintAccumulatedFloatAndExecuteOutstandingShifts if userNextPrice_stakedSyntheticTokenShiftIndex == latestRewardIndex",
      () => {
        let userNextPrice_stakedSyntheticTokenShiftIndex =
          Helpers.randomInteger();
        let latestRewardIndex = userNextPrice_stakedSyntheticTokenShiftIndex;

        let%Await _ =
          setup(
            ~isShiftFromLong,
            ~amountSyntheticTokensToShiftBeforeValue,
            ~userNextPrice_stakedSyntheticTokenShiftIndex,
            ~latestRewardIndex,
            ~amountSyntheticTokensToShift,
            ~userAmountStaked=
              amountSyntheticTokensToShift->add(
                amountSyntheticTokensToShiftBeforeValue,
              ),
          );
        ();

        expect(
          StakerSmocked.InternalMock._mintAccumulatedFloatAndExecuteOutstandingShiftsFunction(),
        )
        ->toHaveCallCount(0);
      },
    );

    it(
      "doesn't call _mintAccumulatedFloatAndExecuteOutstandingShifts if userNextPrice_stakedSyntheticTokenShiftIndex > latestRewardIndex",
      () => {
        let latestRewardIndex = Helpers.randomInteger();
        let userNextPrice_stakedSyntheticTokenShiftIndex =
          latestRewardIndex->add(Helpers.randomInteger());

        let%Await _ =
          setup(
            ~isShiftFromLong,
            ~amountSyntheticTokensToShiftBeforeValue,
            ~userNextPrice_stakedSyntheticTokenShiftIndex,
            ~latestRewardIndex,
            ~amountSyntheticTokensToShift,
            ~userAmountStaked=
              amountSyntheticTokensToShift->add(
                amountSyntheticTokensToShiftBeforeValue,
              ),
          );

        expect(
          StakerSmocked.InternalMock._mintAccumulatedFloatAndExecuteOutstandingShiftsFunction(),
        )
        ->toHaveCallCount(0);
      },
    );

    it(
      "sets the userNextPrice_stakedSyntheticTokenShiftIndex for the user to latestRewardIndex + 1",
      () => {
        let latestRewardIndex = Helpers.randomInteger();
        let user = accounts.contents->Array.getUnsafe(0).address;

        let%Await _ =
          setup(
            ~isShiftFromLong,
            ~amountSyntheticTokensToShiftBeforeValue,
            ~userNextPrice_stakedSyntheticTokenShiftIndex=zeroBn,
            ~latestRewardIndex,
            ~amountSyntheticTokensToShift,
            ~userAmountStaked=
              amountSyntheticTokensToShift->add(
                amountSyntheticTokensToShiftBeforeValue,
              ),
          );

        let%Await userNextPrice_stakedSyntheticTokenShiftIndexAfter =
          contracts.contents.staker
          ->Staker.userNextPrice_stakedSyntheticTokenShiftIndex(
              marketIndex,
              user,
            );

        Chai.bnEqual(
          userNextPrice_stakedSyntheticTokenShiftIndexAfter,
          latestRewardIndex->add(oneBn),
        );
      },
    );

    let sideSpecificTests = (~isShiftFromLong) => {
      it(
        "calls the shiftPositionFrom"
        ++ (isShiftFromLong ? "Long" : "Short")
        ++ "NextPrice function on long short with the correct parameters",
        () => {
          let {longShortSmocked} = contracts.contents;
          let latestRewardIndex = Helpers.randomInteger();

          let%Await _ =
            setup(
              ~isShiftFromLong,
              ~amountSyntheticTokensToShiftBeforeValue,
              ~userNextPrice_stakedSyntheticTokenShiftIndex=zeroBn,
              ~latestRewardIndex,
              ~amountSyntheticTokensToShift,
              ~userAmountStaked=
                amountSyntheticTokensToShift->add(
                  amountSyntheticTokensToShiftBeforeValue,
                ),
            );

          longShortSmocked->LongShortSmocked.shiftPositionNextPriceCallCheck({
            marketIndex,
            amountSyntheticTokensToShift,
            isShiftFromLong,
          });
        },
      );
      it(
        "updates the amountToShiftFrom"
        ++ (isShiftFromLong ? "Long" : "Short")
        ++ "User value with the amount to shift",
        () => {
          let latestRewardIndex = Helpers.randomInteger();
          let user = accounts.contents->Array.getUnsafe(0).address;

          let%Await _ =
            setup(
              ~isShiftFromLong,
              ~amountSyntheticTokensToShiftBeforeValue,
              ~userNextPrice_stakedSyntheticTokenShiftIndex=zeroBn,
              ~latestRewardIndex,
              ~amountSyntheticTokensToShift,
              ~userAmountStaked=
                amountSyntheticTokensToShift->add(
                  amountSyntheticTokensToShiftBeforeValue,
                ),
            );

          let%Await totalAmountToShiftFromSide =
            contracts.contents.staker
            ->Staker.userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom(
                marketIndex,
                isShiftFromLong,
                user,
              );

          Chai.bnEqual(
            totalAmountToShiftFromSide,
            amountSyntheticTokensToShiftBeforeValue->add(
              amountSyntheticTokensToShift,
            ),
          );
        },
      );
    };

    describe("Shift from Long", () =>
      sideSpecificTests(~isShiftFromLong=true)
    );

    describe("Shift from Short", () =>
      sideSpecificTests(~isShiftFromLong=false)
    );
  });
};
