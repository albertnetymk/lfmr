{-|

The abstract syntax tree produced by the parser. Each node carries
meta-information about its type (filled in by
"Typechecker.Typechecker") and its position in the source file
(filled in by "Parser.Parser")

-}

module AST.AST where

import Data.List
import Data.Maybe
import Text.Parsec(SourcePos, SourceName)

import Identifiers
import Types
import AST.Meta as Meta hiding(Closure, Async)

data Program = Program {
  source :: SourceName,
  bundle :: BundleDecl,
  etl :: [EmbedTL],
  imports :: [ImportDecl],
  typedefs :: [Typedef],
  functions :: [Function],
  traits :: [TraitDecl],
  classes :: [ClassDecl]
} deriving (Show)

class Show a => HasMeta a where
    getMeta :: a -> Meta a

    setMeta :: a -> Meta a -> a

    getPos :: a -> SourcePos
    getPos = Meta.getPos . getMeta

    getType :: a -> Type
    getType = Meta.getType . getMeta

    setType :: Type -> a -> a

    hasType :: a -> Type -> Bool
    hasType x ty = if ty == nullType then
                       not $ isPrimitive ty'
                   else
                       ty == ty'
                   where
                     ty' = AST.AST.getType x

    isFree :: a -> Bool
    isFree = Meta.isFree . getMeta

    isCaptured :: a -> Bool
    isCaptured = Meta.isCaptured . getMeta

    makeFree :: a -> a
    makeFree x = let meta = Meta.makeFree (getMeta x)
                 in setMeta x meta

    makeCaptured :: a -> a
    makeCaptured x = let meta = Meta.makeCaptured (getMeta x)
                     in setMeta x meta

    getArrowType :: a -> Type
    getArrowType = Meta.getMetaArrowType . getMeta

    setArrowType :: Type -> a -> a
    setArrowType ty x = let meta = getMeta x
                        in setMeta x $ Meta.setMetaArrowType ty meta

    showWithKind :: a -> String
    showWithKind = show

    hasEnvChange :: a -> Bool
    hasEnvChange x = Meta.hasEnvChange $ getMeta x

    hasCondEnvChange :: a -> Bool
    hasCondEnvChange x = Meta.hasCondEnvChange $ getMeta x

    getEnvChange :: a -> [(Name, Type)]
    getEnvChange x = Meta.getEnvChange $ getMeta x

    setEnvChange :: [(Name, Type)] -> a -> a
    setEnvChange bindings x = let meta = getMeta x
                              in setMeta x $ Meta.setEnvChange bindings meta

    setCondEnvChange :: [(Name, Type)] -> a -> a
    setCondEnvChange bindings x = let meta = getMeta x
                                  in setMeta x $ Meta.setCondEnvChange bindings meta

data EmbedTL = EmbedTL {
      etlmeta   :: Meta EmbedTL,
      etlheader :: String,
      etlbody   :: String
    } deriving (Show)

data BundleDecl = Bundle {
      bmeta :: Meta BundleDecl,
      bname :: QName
    }
  | NoBundle deriving Show

data ImportDecl = Import {
      imeta   :: Meta ImportDecl,
      itarget :: QName
    } deriving (Show)

instance HasMeta ImportDecl where
    getMeta = imeta

    setMeta i m = i{imeta = m}

    setType ty i =
        error "AST.hs: Cannot set the type of an ImportDecl"

data Typedef = Typedef {
   typedefmeta :: Meta Typedef,
   typedefdef  :: Type  -- will be a TypeSynonym, with left and right hand side of definition built in
} deriving (Show)

instance HasMeta Typedef where
    getMeta = typedefmeta

    setMeta i m = i{typedefmeta = m}

    setType ty i =
        error "AST.hs: Cannot set the type of an Typedef"

data HeaderKind = Streaming
                | NonStreaming
                  deriving(Eq, Show)

data FunctionHeader =
    Header {
        kind    :: HeaderKind,
        hname   :: Name,
        htype   :: Type,
        hparams :: [ParamDecl]
    }
    | MatchingHeader {
        kind        :: HeaderKind,
        hname       :: Name,
        htype       :: Type,
        hparamtypes :: [Type],
        hpatterns   :: [Expr],
        hguard      :: Expr
    }deriving(Eq, Show)

setHeaderType ty h = h{htype = ty}

isStreamMethodHeader h = kind h == Streaming

-- MatchingFunction instances should be replaced by regular
-- functions after desugaring
data Function =
    Function {
      funmeta   :: Meta Function,
      funheader :: FunctionHeader,
      funbody   :: Expr
    }
  | MatchingFunction {
      funmeta         :: Meta Function,
      matchfunheaders :: [FunctionHeader],
      matchfunbodies  :: [Expr]
    } deriving (Show)

functionName = hname . funheader
functionParams = hparams . funheader
functionType = htype . funheader

instance Eq Function where
  a == b = (hname . funheader $ a) == (hname . funheader $ b)

instance HasMeta Function where
  getMeta = funmeta
  setMeta f m = f{funmeta = m}
  setType ty f@(Function {funmeta, funheader}) =
      f{funmeta = Meta.setType ty funmeta
       ,funheader = setHeaderType ty funheader}
  setType ty f@(MatchingFunction {funmeta, matchfunheaders}) =
      f{funmeta = Meta.setType ty funmeta
       ,matchfunheaders = map (setHeaderType ty) matchfunheaders}
  showWithKind Function{funheader} =
      "function '" ++ show (hname funheader) ++ "'"
  showWithKind MatchingFunction{matchfunheaders} =
      "function '" ++ show (hname $ head matchfunheaders) ++ "'"

data ClassDecl = Class {
  cmeta       :: Meta ClassDecl,
  cname       :: Type,
  ccapability :: Type,
  cfields     :: [FieldDecl],
  cmethods    :: [MethodDecl]
} deriving (Show)

instance Eq ClassDecl where
  a == b = getId (cname a) == getId (cname b)

isActive :: ClassDecl -> Bool
isActive = isActiveClassType . cname

isShared :: ClassDecl -> Bool
isShared = isSharedClassType . cname

isPassive :: ClassDecl -> Bool
isPassive = isPassiveClassType . cname

isMainClass :: ClassDecl -> Bool
isMainClass cdecl =
    let ty = cname cdecl
    in getId ty == "Main" && isActiveClassType ty


instance HasMeta ClassDecl where
    getMeta = cmeta
    setMeta c m = c{cmeta = m}
    setType ty c@(Class {cmeta, cname}) =
      c {cmeta = Meta.setType ty cmeta, cname = ty}
    showWithKind Class{cname} = "class '" ++ getId cname ++ "'"

data Requirement =
    RequiredField {
       rmeta :: Meta Requirement
      ,rfield :: FieldDecl
    }
    | RequiredMethod {
       rmeta :: Meta Requirement
      ,rheader :: FunctionHeader
    } deriving(Show)

isRequiredField RequiredField{} = True
isRequiredField _ = False

isRequiredMethod RequiredMethod{} = True
isRequiredMethod _ = False

instance Eq Requirement where
    a == b
        | isRequiredField a
        , isRequiredField b =
            rfield a == rfield b
        | isRequiredMethod a
        , isRequiredMethod b =
            rheader a == rheader b
        | otherwise = False

instance HasMeta Requirement where
    getMeta = rmeta
    setMeta r m = r{rmeta = m}
    setType ty r@(RequiredField{rmeta, rfield}) =
      r{rmeta = Meta.setType ty rmeta, rfield = AST.AST.setType ty rfield}
    setType ty r@(RequiredMethod{rmeta, rheader}) =
      r{rmeta = Meta.setType ty rmeta, rheader = setHeaderType ty rheader}
    showWithKind RequiredField{rfield} =
        "required field '" ++ show rfield ++ "'"
    showWithKind RequiredMethod{rheader} =
        "required method '" ++ show (hname rheader) ++ "'"

data TraitDecl = Trait {
  tmeta :: Meta TraitDecl,
  tname :: Type,
  treqs :: [Requirement],
  tmethods :: [MethodDecl]
} deriving (Show)

requiredFields :: TraitDecl -> [FieldDecl]
requiredFields Trait{treqs} =
    map rfield $ filter isRequiredField treqs

requiredMethods :: TraitDecl -> [FunctionHeader]
requiredMethods Trait{treqs} =
    map rheader $ filter isRequiredMethod treqs

traitInterface :: TraitDecl -> [FunctionHeader]
traitInterface t@Trait{tmethods} =
    requiredMethods t ++ map mheader tmethods

instance Eq TraitDecl where
  a == b = getId (tname a) == getId (tname b)

instance HasMeta TraitDecl where
  getMeta = tmeta
  setMeta t m = t{tmeta = m}
  setType ty t@Trait{tmeta, tname} =
    t{tmeta = Meta.setType ty tmeta, tname = ty}
  showWithKind Trait{tname} = "trait '" ++ getId tname ++ "'"

data Modifier = Val
              | Spec
              | Once
                deriving(Eq)

instance Show Modifier where
    show Val  = "val"
    show Spec = "spec"
    show Once = "once"

data FieldDecl = Field {
  fmeta :: Meta FieldDecl,
  fmods :: [Modifier],
  fname :: Name,
  ftype :: Type
}

instance Show FieldDecl where
  show f@Field{fmods,fname,ftype} =
      smods ++ show fname ++ " : " ++ show ftype
    where
      smods = concatMap ((++ " ") . show) fmods

instance Eq FieldDecl where
  a == b = fname a == fname b

instance HasMeta FieldDecl where
    getMeta = fmeta
    setMeta f m = f{fmeta = m}
    setType ty f@(Field {fmeta, ftype}) = f {fmeta = Meta.setType ty fmeta, ftype = ty}
    showWithKind Field{fname} = "field '" ++ show fname ++ "'"

isOnceField :: FieldDecl -> Bool
isOnceField = (Once `elem`) . fmods

isSpecField :: FieldDecl -> Bool
isSpecField = (Spec `elem`) . fmods

isValField :: FieldDecl -> Bool
isValField = (Val `elem`) . fmods

isSafeValField :: FieldDecl -> Bool
isSafeValField f@Field{ftype} = isValField f && isSafeType ftype

isVarField :: FieldDecl -> Bool
isVarField = null . fmods

data ParamDecl = Param {
  pmeta :: Meta ParamDecl,
  pname :: Name,
  ptype :: Type
} deriving (Show, Eq)

instance HasMeta ParamDecl where
    getMeta = pmeta
    setMeta p m = p{pmeta = m}
    setType ty p@(Param {pmeta, ptype}) = p {pmeta = Meta.setType ty pmeta, ptype = ty}
    showWithKind Param{pname} = "parameter '" ++ show pname ++ "'"

data MethodDecl =
    Method {
      mmeta   :: Meta MethodDecl,
      mheader :: FunctionHeader,
      mbody   :: Expr}
  | MatchingMethod {
      mmeta    :: Meta MethodDecl,
      mheaders :: [FunctionHeader],
      mbodies  :: [Expr]
    } deriving (Show)

methodName = hname . mheader
methodParams = hparams . mheader
methodType = htype . mheader

isStreamMethod Method{mheader} = isStreamMethodHeader mheader

isMainMethod :: Type -> Name -> Bool
isMainMethod ty name = isMainType ty && (name == Name "main")

isConstructor :: MethodDecl -> Bool
isConstructor m = methodName m == constructorName

emptyConstructor :: ClassDecl -> MethodDecl
emptyConstructor cdecl =
    let pos = AST.AST.getPos cdecl
    in Method{mmeta = meta pos
             ,mheader = Header{kind = NonStreaming
                              ,hname = Name "_init"
                              ,hparams = []
                              ,htype = voidType
                              }
             ,mbody = Skip (meta pos)}

replaceHeaderTypes :: [(Type, Type)] -> FunctionHeader -> FunctionHeader
replaceHeaderTypes bindings header =
    let hparams' = map (replaceParamType bindings) (hparams header)
        htype' = replaceTypeVars bindings (htype header)
    in
      header{hparams = hparams', htype = htype'}
    where
      replaceParamType bindings p@Param{ptype} =
          p{ptype = replaceTypeVars bindings ptype}

instance Eq MethodDecl where
  a == b = methodName a == methodName b

instance HasMeta MethodDecl where
  getMeta = mmeta
  setMeta mtd m = mtd{mmeta = m}
  setType ty m =
      let header = mheader m
          meta = mmeta m
      in
        m{mmeta = Meta.setType ty meta
         ,mheader = setHeaderType ty header}
  showWithKind m
      | isStreamMethod m = "streaming method '" ++ show (methodName m) ++ "'"
      | otherwise = "method '" ++ show (methodName m) ++ "'"

data MatchClause =
    MatchClause {
      mcpattern :: Expr,
      mchandler :: Expr,
      mcguard   :: Expr
    } deriving (Show, Eq)

type Arguments = [Expr]

data MaybeContainer = JustData { e :: Expr}
                    | NothingData deriving(Eq, Show)

data VarDecl =
    Var {varName :: Name}
  | VarDecl {varName :: Name,
             varType :: Type}
  deriving(Eq, Show)

data Expr = Skip {emeta :: Meta Expr}
          | Return {emeta :: Meta Expr,
                    val :: Expr}
          | Break {emeta :: Meta Expr}
          | TypedExpr {emeta :: Meta Expr,
                       body :: Expr,
                       ty   :: Type}
          | MethodCall {emeta :: Meta Expr,
                        target :: Expr,
                        name :: Name,
                        args :: Arguments}
          | MessageSend {emeta :: Meta Expr,
                         target :: Expr,
                         name :: Name,
                         args :: Arguments}
          | FunctionCall {emeta :: Meta Expr,
                          name :: Name,
                          args :: Arguments}
          | Closure {emeta :: Meta Expr,
                     eparams :: [ParamDecl],
                     body :: Expr}
          | Liftf {emeta :: Meta Expr,
                   val :: Expr}
          | Liftv {emeta :: Meta Expr,
                   val :: Expr}
          | PartyJoin {emeta :: Meta Expr,
                       val :: Expr}
          | PartyExtract {emeta :: Meta Expr,
                          val :: Expr}
          | PartyEach {emeta :: Meta Expr,
                       val :: Expr}
          | PartySeq {emeta :: Meta Expr,
                      par :: Expr,
                      seqfunc :: Expr}
          | PartyPar {emeta :: Meta Expr,
                      parl :: Expr,
                      parr :: Expr}
          | Async {emeta :: Meta Expr,
                   body :: Expr}
          | MaybeValue {emeta :: Meta Expr,
                        mdt :: MaybeContainer }
          | Tuple {emeta :: Meta Expr,
                   args :: [Expr]}
          | Foreach {emeta :: Meta Expr,
                     item :: Name,
                     arr :: Expr,
                     body :: Expr}
          | FinishAsync {emeta :: Meta Expr,
                         body :: Expr}
          | Let {emeta :: Meta Expr,
                 decls :: [([VarDecl], Expr)],
                 body :: Expr}
          | MiniLet {emeta :: Meta Expr,
                     decl :: ([VarDecl], Expr)}
          | Seq {emeta :: Meta Expr,
                 eseq :: [Expr]}
          | IfThenElse {emeta :: Meta Expr,
                        cond :: Expr,
                        thn :: Expr,
                        els :: Expr}
          | IfThen {emeta :: Meta Expr,
                    cond :: Expr,
                    thn :: Expr}
          | Unless {emeta :: Meta Expr,
                    cond :: Expr,
                    thn :: Expr}
          | While {emeta :: Meta Expr,
                   cond :: Expr,
                   body :: Expr}
          | Repeat {emeta :: Meta Expr,
                    name :: Name,
                    times :: Expr,
                    body :: Expr}
          | For {emeta  :: Meta Expr,
                 name   :: Name,
                 step   :: Expr,
                 src    :: Expr,
                 body   :: Expr}
          | Match {emeta :: Meta Expr,
                   arg :: Expr,
                   clauses :: [MatchClause]}
          | CAT {emeta :: Meta Expr,
                 args  :: [Expr],
                 names :: [Name]
                }
          | TryAssign {emeta :: Meta Expr,
                       target :: Expr,
                       arg :: Expr
                      }
          | Freeze {emeta :: Meta Expr,
                    target :: Expr}
          | IsFrozen {emeta :: Meta Expr,
                      target :: Expr}
          | Get {emeta :: Meta Expr,
                 val :: Expr}
          | Yield {emeta :: Meta Expr,
                   val :: Expr}
          | Eos {emeta :: Meta Expr}
          | IsEos {emeta :: Meta Expr,
                   target :: Expr}
          | StreamNext {emeta :: Meta Expr,
                        target :: Expr}
          | Await {emeta :: Meta Expr,
                   val :: Expr}
          | Suspend {emeta :: Meta Expr}
          | FutureChain {emeta :: Meta Expr,
                        future :: Expr,
                         chain :: Expr}
          | FieldAccess {emeta :: Meta Expr,
                         target :: Expr,
                         name :: Name}
          | ArrayAccess {emeta :: Meta Expr,
                         target :: Expr,
                         index :: Expr}
          | ArraySize {emeta :: Meta Expr,
                       target :: Expr}
          | ArrayNew {emeta :: Meta Expr,
                      ty :: Type,
                      size :: Expr}
          | ArrayLiteral {emeta :: Meta Expr,
                          args :: [Expr]}
          | Assign {emeta :: Meta Expr,
                    lhs :: Expr,
                    rhs :: Expr}
          | VarAccess {emeta :: Meta Expr,
                       name :: Name}
          | Consume {emeta :: Meta Expr,
                     target :: Expr}
          | Null {emeta :: Meta Expr}
          | BTrue {emeta :: Meta Expr}
          | BFalse {emeta :: Meta Expr}
          | NewWithInit {emeta :: Meta Expr,
                         ty ::Type,
                         args :: Arguments}
          | New {emeta :: Meta Expr,
                 ty ::Type}
          | Peer {emeta :: Meta Expr,
                  ty ::Type}
          | Print {emeta :: Meta Expr,
                   args :: [Expr]}
          | Speculate {emeta :: Meta Expr,
                       arg :: Expr}
          | Exit {emeta :: Meta Expr,
                  args :: [Expr]}
          | StringLiteral {emeta :: Meta Expr,
                           stringLit :: String}
          | CharLiteral {emeta :: Meta Expr,
                         charLit :: Char}
          | RangeLiteral {emeta :: Meta Expr,
                          start  :: Expr,
                          stop   :: Expr,
                          step   :: Expr}
          | IntLiteral {emeta :: Meta Expr,
                        intLit :: Int}
          | RealLiteral {emeta :: Meta Expr,
                         realLit :: Double}
          | Embed {emeta :: Meta Expr,
                   ty    :: Type,
                   code  :: String}
          | Unary {emeta :: Meta Expr,
                   uop   :: UnaryOp,
                   operand  :: Expr }
          | Binop {emeta :: Meta Expr,
                   binop :: BinaryOp,
                   loper :: Expr,
                   roper :: Expr} deriving(Show, Eq)

isLval :: Expr -> Bool
isLval VarAccess {} = True
isLval FieldAccess {} = True
isLval ArrayAccess {} = True
isLval _ = False

isVarAccess :: Expr -> Bool
isVarAccess VarAccess {} = True
isVarAccess _ = False

isThisAccess :: Expr -> Bool
isThisAccess VarAccess {name = Name "this"} = True
isThisAccess _ = False

isFieldAccess :: Expr -> Bool
isFieldAccess FieldAccess {} = True
isFieldAccess _ = False

isClosure :: Expr -> Bool
isClosure Closure {} = True
isClosure _ = False

isNull :: Expr -> Bool
isNull Null{} = True
isNull _ = False

isTask :: Expr -> Bool
isTask Async {} = True
isTask _ = False

isRangeLiteral :: Expr -> Bool
isRangeLiteral RangeLiteral {} = True
isRangeLiteral _ = False

isCallable :: Expr -> Bool
isCallable e = isArrowType (AST.AST.getType e)

isStringLiteral :: Expr -> Bool
isStringLiteral StringLiteral {} = True
isStringLiteral _ = False

isPrimitiveLiteral :: Expr -> Bool
isPrimitiveLiteral Skip{}          = True
isPrimitiveLiteral BTrue{}         = True
isPrimitiveLiteral BFalse{}        = True
isPrimitiveLiteral StringLiteral{} = True
isPrimitiveLiteral NewWithInit{ty} = isStringObjectType ty
isPrimitiveLiteral CharLiteral{}   = True
isPrimitiveLiteral IntLiteral{}    = True
isPrimitiveLiteral RealLiteral{}   = True
isPrimitiveLiteral Unary{uop = NEG, operand} = isPrimitiveLiteral operand
isPrimitiveLiteral _ = False

isPattern :: Expr -> Bool
isPattern TypedExpr{body} = isPattern body
isPattern FunctionCall{} = True
isPattern MaybeValue{mdt = JustData{e}} = isPattern e
isPattern MaybeValue{mdt = NothingData} = True
isPattern Tuple{args} = all isPattern args
isPattern VarAccess{} = True
isPattern Null{} = True
isPattern e
    | isPrimitiveLiteral e = True
    | otherwise = False

instance HasMeta Expr where
    getMeta = emeta
    setMeta e m = e{emeta = m}

    hasType (Null {}) ty = not . isPrimitive $ ty
    hasType x ty = if ty == nullType then
                       not $ isPrimitive ty'
                   else
                       ty == ty'
                   where
                     ty' = AST.AST.getType x

    setType ty expr = expr {emeta = Meta.setType ty (emeta expr)}

setSugared :: Expr -> Expr -> Expr
setSugared e sugared = e {emeta = Meta.setSugared sugared (emeta e)}

getSugared :: Expr -> Maybe Expr
getSugared e = Meta.getSugared (emeta e)

traverseProgram :: (Program -> [a]) -> Program -> [a]
traverseProgram f program = f program

getTrait :: Type -> Program -> TraitDecl
getTrait t p =
  let
    traits = allTraits p
    match t trait = getId t == getId (tname trait)
  in
    fromJust $ find (match t) traits

getClass :: Type -> Program -> ClassDecl
getClass t p =
  let
    classes = allClasses p
    match l r = getId l == getId (cname r)
  in
    fromJust $ find (match t) classes

allTypedefs = traverseProgram typedefs

allClasses = traverseProgram classes

allTraits = traverseProgram traits

allFunctions = traverseProgram functions

allEmbedded = traverseProgram (map etlheader . etl)
