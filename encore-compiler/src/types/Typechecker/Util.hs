{-|
  Utility functions shared by several modules of "Typechecker".
-}

module Typechecker.Util(TypecheckM
                       ,CapturecheckM
                       ,whenM
                       ,anyM
                       ,allM
                       ,unlessM
                       ,tcError
                       ,pushError
                       ,tcWarning
                       ,resolveType
                       ,resolveTypeAndCheckForLoops
                       ,subtypeOf
                       ,assertSubtypeOf
                       ,assertCanFlowInto
                       ,canFlowInto
                       ,assertDistinctThing
                       ,assertDistinct
                       ,findField
                       ,findMethod
                       ,findMethodWithCalledType
                       ,findCapability
                       ,propagateResultType
                       ,isLinearType
                       ,isSubordinateType
                       ,isEncapsulatedType
                       ,isThreadType
                       ,isAliasableType
                       ,checkConjunction
                       ,isSpineType
                       ) where

import Identifiers
import Types as Ty
import AST.AST as AST
import Data.List
import Data.Maybe
import Text.Printf (printf)
import Debug.Trace
import Control.Monad.Reader
import Control.Monad.Except
import Control.Arrow(second)
import Control.Monad.State

-- Module dependencies
import Typechecker.TypeError
import Typechecker.Environment
import AST.PrettyPrinter

-- Monadic versions of common functions
anyM :: (Monad m) => (a -> m Bool) -> [a] -> m Bool
anyM p = foldM (\b x -> liftM (b ||) (p x)) False

allM :: (Monad m) => (a -> m Bool) -> [a] -> m Bool
allM p = foldM (\b x -> liftM (b &&) (p x)) True

whenM :: (Monad m) => m Bool -> m () -> m ()
whenM cond action = cond >>= (`when` action)

unlessM :: (Monad m) => m Bool -> m () -> m ()
unlessM cond action = cond >>= (`unless` action)

-- | The monad in which all typechecking is performed. A function
-- of return type @TypecheckM Bar@ may read from an 'Environment'
-- and returns a @Bar@ or throws a typechecking exception.
type TypecheckM a =
    forall m . (MonadState [TCWarning] m,
                MonadError TCError m,
                MonadReader Environment m) => m a

type CapturecheckM a =
    forall m . (MonadError CCError m,
                MonadReader Environment m) => m a

-- | Convenience function for throwing an exception with the
-- current backtrace
tcError err =
    do bt <- asks backtrace
       throwError $ TCError err bt

-- | Push the expression @expr@ and throw error err
pushError expr err = local (pushBT expr) $ tcError err

tcWarning wrn =
    do bt <- asks backtrace
       modify (TCWarning bt wrn:)

-- | @matchTypeParameterLength ty1 ty2@ ensures that the type parameter
-- lists of its arguments have the same length.
matchTypeParameterLength :: Type -> Type -> TypecheckM ()
matchTypeParameterLength ty1 ty2 = do
  let params1 = getTypeParameters ty1
      params2 = getTypeParameters ty2
  unless (length params1 == length params2) $
    tcError $ WrongNumberOfTypeParametersError
              ty1 (length params1) ty2 (length params2)

-- | @resolveType ty@ checks all the components of @ty@, resolving
-- reference types to traits or classes and making sure that any
-- type variables are in the current environment.
resolveType :: Type -> TypecheckM Type
resolveType = typeMapM (\ty -> do ty' <- resolveSingleType ty
                                  checkRestrictions ty'
                                  return ty')

checkRestrictions :: Type -> TypecheckM ()
checkRestrictions ty
  | isClassType ty = do
      mapM_ checkBarRestriction (barredFields ty)
      mapM_ checkTransferRestriction (transferRestrictedFields ty)
  | otherwise =
      unless (null $ restrictedFields ty) $
             tcError $ CannotHaveRestrictedFieldsError ty
  where
    checkBarRestriction f = do
      fdecl <- findField (ty `unrestrict` f) f
      unless (isVarField fdecl) $
             tcError $ BarredNonVarFieldError fdecl
    checkTransferRestriction f = do
      fdecl <- findField (ty `unrestrict` f) f
      when (isVarField fdecl) $
           tcError $ TransferRestrictedVarFieldError fdecl


resolveSingleType :: Type -> TypecheckM Type
resolveSingleType ty
  | isTypeVar ty = do
      params <- asks typeParameters
      unless (ty `elem` params) $
             tcError $ FreeTypeVariableError ty
      return ty
  | isRefType ty = do
      (res, formal) <- resolveRefType ty
      if isTypeSynonym res
      then resolveType res -- Force unfolding of type synonyms
      else resolveMode res formal
  | isCapabilityType ty =
      resolveCapa ty
  | isStringType ty = do
      tcWarning StringDeprecatedWarning
      return ty
  | isTypeSynonym ty = do
      unless (isModeless ty) $
        tcError $ CannotHaveModeError ty
      let unfolded = unfoldTypeSynonyms ty
      resolveType unfolded
  | otherwise = return ty
  where
    resolveCapa t =
        mapM_ resolveSingleTrait (typesFromCapability t) >> return t
    resolveSingleTrait t
      | isRefType t = do
          result <- asks $ traitLookup t
          when (isNothing result) $
             tcError $ UnknownTraitError t
      | otherwise =
          tcError $ MalformedCapabilityError t

resolveTypeAndCheckForLoops :: Type -> TypecheckM Type
resolveTypeAndCheckForLoops ty =
  evalStateT (typeMapM resolveAndCheck ty) []
  where
    resolveAndCheck ty
      | isRefType ty = do
          seen <- get
          let tyid = getId ty
          when (tyid `elem` seen) $
            lift . tcError $ RecursiveTypesynonymError ty
          (res, formal) <- lift $ resolveRefType ty
          when (isTypeSynonym res) $ put (tyid : seen)
          if isTypeSynonym res
          then typeMapM resolveAndCheck res
          else lift $ resolveMode res formal
      | otherwise = lift $ resolveType ty

resolveRefType :: Type -> TypecheckM (Type, Type)
resolveRefType ty
  | isRefType ty = do
      result <- asks $ refTypeLookup ty
      case result of
        Just formal -> do
          matchTypeParameterLength formal ty
          let res = ty `resolvedFrom` formal
          return (res, formal)
        Nothing ->
          tcError $ UnknownRefTypeError ty
  | otherwise = error $ "Util.hs: " ++ Ty.showWithKind ty ++ " isn't a ref-type"

resolveMode :: Type -> Type -> TypecheckM Type
resolveMode actual formal
  | isModeless actual && not (isModeless formal) =
      resolveMode (actual `withModeOf` formal) formal
  | isClassType actual = do
      unless (isModeless actual) $
        tcError $ CannotHaveModeError actual
      return actual
  | isTraitType actual = do
      when (isModeless actual) $
           tcError $ ModelessError actual
      unless (isModeless formal || actual `modeSubtypeOf` formal) $
           tcError $ ModeOverrideError formal
      when (isReadRefType actual) $
           unless (isReadRefType formal) $
                  tcError CannotGiveReadModeError
      return actual
  | otherwise =
      error $ "Util.hs: Cannot resolve unknown reftype: " ++ show formal

subtypeOf :: Type -> Type -> TypecheckM Bool
subtypeOf ty1 ty2
    | isArrowType ty1 && isArrowType ty2 = do
        let argTys1 = getArgTypes ty1
            argTys2 = getArgTypes ty2
            resultTy1 = getResultType ty1
            resultTy2 = getResultType ty2
        contravariance <- liftM and $ zipWithM subtypeOf argTys2 argTys1
        covariance <- resultTy1 `subtypeOf` resultTy2
        return $ length argTys1 == length argTys2 &&
                 contravariance && covariance
    | hasResultType ty1 && hasResultType ty2 =
        liftM (ty1 `hasSameKind` ty2 &&) $
              getResultType ty1 `subtypeOf` getResultType ty2
    | isNullType ty1 = return (isNullType ty2 || isRefType ty2)
    | isClassType ty1 && isClassType ty2 =
        ty1 `refSubtypeOf` ty2
    | isClassType ty1 && isTraitType ty2 = do
        traits <- getImplementedTraits ty1
        anyM (`subtypeOf` ty2) traits
    | isClassType ty1 && isCapabilityType ty2 = do
        capability <- findCapability ty1
        capability `capabilitySubtypeOf` ty2
    | isTupleType ty1 && isTupleType ty2 = do
      let argTys1 = getArgTypes ty1
          argTys2 = getArgTypes ty2
      results <- zipWithM subtypeOf argTys1 argTys2
      return $ and results && length argTys1 == length argTys2
    | isTraitType ty1 && isTraitType ty2 =
        liftM (ty1 `modeSubtypeOf` ty2 &&)
              (ty1 `refSubtypeOf` ty2)
    | isTraitType ty1 && isCapabilityType ty2 = do
        let traits = typesFromCapability ty2
        allM (ty1 `subtypeOf`) traits
    | isCapabilityType ty1 && isTraitType ty2 = do
        let traits = typesFromCapability ty1
        anyM (`subtypeOf` ty2) traits
    | isCapabilityType ty1 && isCapabilityType ty2 =
        ty1 `capabilitySubtypeOf` ty2
    | isBottomType ty1 && (not . isBottomType $ ty2) = return True
    | isNumeric ty1 && isNumeric ty2 =
        return $ ty1 `numericSubtypeOf` ty2
    | otherwise = return (ty1 == ty2)
    where
      refSubtypeOf ref1 ref2
          | getId ref1 == getId ref2
          , params1 <- getTypeParameters ref1
          , params2 <- getTypeParameters ref2
          , length params1 == length params2 = do
              results <- zipWithM subtypeOf params1 params2
              return $ and results && matchingPristiness ref1 ref2
          | otherwise = return False
          where
            matchingPristiness ref1 ref2
                | isPristineRefType ref1 = True
                | not (isPristineRefType ref1) && not (isPristineRefType ref2) = True
                | otherwise = False

      capabilitySubtypeOf cap1 cap2 = do
        let traits1 = typesFromCapability cap1
            traits2 = typesFromCapability cap2
            preservesConjunctions = cap1 `preservesConjunctionsOf` cap2
        isSubsumed <- allM (\t2 -> anyM (`subtypeOf` t2) traits1) traits2
        return (preservesConjunctions && isSubsumed)

      preservesConjunctionsOf cap1 cap2 =
        let pairs1 = conjunctiveTypesFromCapability cap1
            pairs2 = conjunctiveTypesFromCapability cap2
        in all (`existsIn` pairs1) pairs2
      existsIn (left, right) =
        any (separates left right)
      separates left right (l, r) =
        all (`elem` l) left && all (`elem` r) right ||
        all (`elem` l) right && all (`elem` r) left

      numericSubtypeOf ty1 ty2
          | isIntType ty1 && isRealType ty2 = True
          | otherwise = ty1 == ty2

assertSubtypeOf :: Type -> Type -> TypecheckM ()
assertSubtypeOf sub super =
    unlessM (sub `subtypeOf` super) $ do
      capability <- if isClassType sub
                    then do
                      cap <- asks $ capabilityLookup sub
                      if maybe False (not . isIncapability) cap
                      then return cap
                      else return Nothing
                    else return Nothing
      case capability of
        Just cap ->
            tcError $ TypeWithCapabilityMismatchError sub cap super
        Nothing ->
            tcError $ TypeMismatchError sub super

canFlowInto :: Type -> Type -> TypecheckM Bool
canFlowInto ty1 ty2 = do
  isSubtype <- ty1 `subtypeOf` ty2
  let restricted1 = restrictedFields ty1
      restricted2 = restrictedFields ty2
      strong1 = stronglyRestrictedFields ty1
      strong2 = stronglyRestrictedFields ty2
      weak2 = weaklyRestrictedFields ty2
      restrictionsPreserved = null (restricted1 \\ restricted2)
      strongRestrictionsOK = all (`elem` weak2) strong1 &&
                             not (any (`elem` strong2) strong1)
                             -- The second condition is just a precaution...
  return $ isSubtype && restrictionsPreserved && strongRestrictionsOK

assertCanFlowInto :: Type -> Type -> TypecheckM ()
assertCanFlowInto ty1 ty2 = do
  ty1 `assertSubtypeOf` ty2
  unlessM (ty1 `canFlowInto` ty2) $
          tcError $ CannotFlowIntoError ty1 ty2

-- | Convenience function for asserting distinctness of a list of
-- things. @assertDistinct "declaration" "field" [f : Foo, f :
-- Bar]@ will throw an error with the message "Duplicate
-- declaration of field 'f'".
assertDistinctThing :: (Eq a, Show a) =>
                       String -> String -> [a] -> TypecheckM ()
assertDistinctThing something kind l =
  let
    duplicates = l \\ nub l
    duplicate = head duplicates
  in
    unless (null duplicates) $
      tcError $ DuplicateThingError something (kind ++ show duplicate)

-- | Convenience function for asserting distinctness of a list of
-- things that @HasMeta@ (and thus knows how to print its own
-- kind). @assertDistinct "declaration" [f : Foo, f : Bar]@ will
-- throw an error with the message "Duplicate declaration of field
-- 'f'".
assertDistinct :: (Eq a, AST.HasMeta a) =>
                  String -> [a] -> TypecheckM ()
assertDistinct something l =
  let
    duplicates = l \\ nub l
    first = head duplicates
  in
    unless (null duplicates) $
      tcError $ DuplicateThingError something (AST.showWithKind first)

findField :: Type -> Name -> TypecheckM FieldDecl
findField ty f = do
  result <- asks $ fieldLookup ty f
  case result of
    Just fdecl -> return fdecl
    Nothing ->
        if f `elem` barredFields ty
        then tcError $ RestrictedFieldLookupError f ty
        else tcError $ FieldNotFoundError f ty

findMethod :: Type -> Name -> TypecheckM FunctionHeader
findMethod ty = liftM fst . findMethodWithCalledType ty

findMethodWithCalledType :: Type -> Name -> TypecheckM (FunctionHeader, Type)
findMethodWithCalledType ty name = do
  result <- asks $ methodAndCalledTypeLookup ty name
  when (isNothing result) $
    tcError $ MethodNotFoundError name ty
  return $ fromJust result

findCapability :: (MonadReader Environment m) => Type -> m Type
findCapability ty = do
  result <- asks $ capabilityLookup ty
  return $ fromMaybe err result
    where
        err = error $ "Util.hs: No capability in " ++ Ty.showWithKind ty

getImplementedTraits :: Type -> TypecheckM [Type]
getImplementedTraits ty
    | isClassType ty = do
        capability <- findCapability ty
        return $ typesFromCapability capability
    | otherwise =
        error $ "Types.hs: Can't get implemented traits of type " ++ show ty

propagateResultType :: Type -> Expr -> Expr
propagateResultType ty e
    | hasResultingBody e =
        let body' = propagateResultType ty (body e)
        in setType ty e{body = body'}
    | Match{clauses} <- e =
        let clauses' = map propagateMatchClause clauses
        in setType ty e{clauses = clauses'}
    | Seq{eseq} <- e =
        let result = propagateResultType ty (last eseq)
        in setType ty e{eseq = init eseq ++ [result]}
    | IfThenElse{thn, els} <- e =
        setType ty e{thn = propagateResultType ty thn
                    ,els = propagateResultType ty els}
    | otherwise = setType ty e
    where
      hasResultingBody TypedExpr{} = True
      hasResultingBody Let{} = True
      hasResultingBody While{} = True
      hasResultingBody For{} = True
      hasResultingBody _ = False

      propagateMatchClause mc@MatchClause{mchandler} =
          mc{mchandler = propagateResultType ty mchandler}

isLinearType :: (MonadReader Environment m) => Type -> m Bool
isLinearType ty
    | isPristineRefType ty = return True
    | isRefType ty = do
        Just flds <- asks $ fields ty
        if all (\f -> isValField f || isSpecField f) flds then
            return False
        else
            isLinearType' [] ty
    | otherwise = isLinearType' [] ty
    where
      isLinearType' :: (MonadReader Environment m) => [Type] -> Type -> m Bool
      isLinearType' checked ty = do
        let components = typeComponents (dropArrows ty)
            unchecked = components \\ checked
            classes = filter isClassType unchecked
        capabilities <- mapM findCapability classes
        liftM2 (||)
              (anyM isDirectlyLinear unchecked)
              (anyM (isLinearType' (checked ++ unchecked)) capabilities)

      isDirectlyLinear :: (MonadReader Environment m) => Type -> m Bool
      isDirectlyLinear ty
          | isClassType ty = do
              cap <- findCapability ty
              let components = typeComponents (dropArrows ty) ++
                               typeComponents (dropArrows cap)
              return $ any isLinearRefType components
          | otherwise = do
              let components = typeComponents (dropArrows ty)
              return $ any isLinearRefType components

      dropArrows = typeMap dropArrow
      dropArrow ty
          | isArrowType ty = voidType
          | otherwise = ty

isSubordinateType :: Type -> TypecheckM Bool
isSubordinateType ty
    | isCompositeType ty
    , traits <- typesFromCapability ty
      = anyM isSubordinateType traits
    | isPassiveClassType ty = do
        capability <- findCapability ty
        isSubordinateType capability
    | hasResultType ty =
        isSubordinateType (getResultType ty)
    | isTupleType ty =
        anyM isSubordinateType (getArgTypes ty)
    | otherwise = return $ isSubordinateRefType ty

isEncapsulatedType :: Type -> TypecheckM Bool
isEncapsulatedType ty
    | isCompositeType ty
    , traits <- typesFromCapability ty
      = allM isSubordinateType traits
    | isPassiveClassType ty = do
        capability <- findCapability ty
        isEncapsulatedType capability
    | hasResultType ty =
        isEncapsulatedType (getResultType ty)
    | isTupleType ty =
        allM isEncapsulatedType (getArgTypes ty)
    | otherwise = return $ isSubordinateRefType ty

isThreadType :: Type -> TypecheckM Bool
isThreadType ty
    | isCompositeType ty
    , traits <- typesFromCapability ty
      = anyM isThreadType traits
    | isPassiveClassType ty = do
        capability <- findCapability ty
        isThreadType capability
    | hasResultType ty =
        isThreadType (getResultType ty)
    | isTupleType ty =
        anyM isThreadType (getArgTypes ty)
    | otherwise = return $ isThreadRefType ty

isSpineType :: Type -> TypecheckM Bool
isSpineType ty
    | isCompositeType ty
    , traits <- typesFromCapability ty
      = allM isSpineType traits
    | isPassiveClassType ty = do
        capability <- findCapability ty
        isSpineType capability
    | hasResultType ty =
        isSpineType (getResultType ty)
    | isTupleType ty =
        anyM isSpineType (getArgTypes ty)
    | otherwise = return $ isSpineRefType ty

isUnsafeType :: Type -> TypecheckM Bool
isUnsafeType ty
    | isPassiveClassType ty = do
        capability <- findCapability ty
        isUnsafeType capability
    | otherwise = return $
                  any isUnsafeRefType $ typeComponents ty

isAliasableType :: Type -> TypecheckM Bool
isAliasableType ty =
    anyM (\f -> f ty)
         [return . isSafeType
         ,isThreadType
         ,isSubordinateType
         ,isUnsafeType
         ]

checkConjunction :: Type -> [Type] -> TypecheckM ()
checkConjunction source sinks
  | isCompositeType source = do
      let sourceConjunctions = conjunctiveTypesFromCapability source
      mapM_ (\ty -> wellFormedConjunction sourceConjunctions
                                          (sinks \\ [ty]) ty) sinks
  | isPassiveClassType source = do
      cap <- findCapability source
      when (isIncapability cap) $
           tcError $ CannotUnpackError source
      when (source `elem` sinks) $
           tcError $ CannotInferUnpackingError source
      checkConjunction cap sinks
  | otherwise =
      return ()
  where
    wellFormedConjunction pairs siblings ty = do
      when (null pairs) $
        tcError $ MalformedConjunctionError ty (head siblings) source
      let nonDisjoints =
            filter (\ty' -> any (not . singleConjunction ty ty') pairs) siblings
          nonDisjoint = head nonDisjoints
      unless (null nonDisjoints) $
        tcError $ MalformedConjunctionError ty nonDisjoint source
    singleConjunction ty1 ty2 (tys1, tys2) =
        ty1 `elem` tys1 && ty2 `elem` tys2 ||
        ty1 `elem` tys2 && ty2 `elem` tys1