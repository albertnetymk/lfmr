module CodeGen.ClassTable (
  ClassTable,
  lookupMethod,
  lookupMethods,
  lookupField,
  lookupCalledType,
  buildClassTable,
  lookup_implemented_capa,
  ) where

import Types
import AST.AST
import Identifiers

import Data.List
import Data.Maybe
import qualified Data.Map.Strict as M
import Control.Arrow

type FieldTable  = [(Name, FieldDecl)]
type MethodTable = [(Name, FunctionHeader)]

data ClassTable = ClassTable {
  program :: Program,
  table :: [(Type, (FieldTable, MethodTable))]
}

buildClassTable :: Program -> ClassTable
buildClassTable p =
  ClassTable{program = p, table = traverseProgram getEntries p}
  where
    getEntries p = map getClassEntry (classes p) ++
                   map getTraitEntry (traits p)
    getClassEntry Class{cname, cfields, cmethods} =
      (cname, (map getFieldEntry cfields,
               map getMethodEntry cmethods))
    getTraitEntry Trait{tname, treqs, tmethods} =
        let (reqFields, reqMethods) = partition isRequiredField treqs
            fieldTable  = map (getFieldEntry . rfield) reqFields
            methodTable = map getReqMethodEntry reqMethods ++
                          map getMethodEntry tmethods
        in
          (tname, (fieldTable, methodTable))
    getFieldEntry f     = (fname f, f)
    getReqMethodEntry r = (hname . rheader $ r, rheader r)
    getMethodEntry m    = (methodName m, mheader m)

lookupEntry :: Type -> ClassTable -> (FieldTable, MethodTable)
lookupEntry ty ctable =
    let fail = error $ "ClassTable.hs: No entry for " ++ Types.showWithKind ty
    in snd $
       fromMaybe fail $ find ((== getId ty) . getId . fst) $ table ctable

lookupField :: Type -> Name -> ClassTable -> FieldDecl
lookupField ty f ctable =
    let (fs, _) = lookupEntry ty ctable
        fail = error $ "ClassTable.hs: No field '" ++ show f ++ "' in " ++
                       Types.showWithKind ty
    in fromMaybe fail $ lookup f fs

lookupMethod :: Type -> Name -> ClassTable -> FunctionHeader
lookupMethod ty m ctable =
    let (_, ms) = lookupEntry ty ctable
        fail = error $ "ClassTable.hs: No method '" ++ show m ++ "' in " ++
                       Types.showWithKind ty
    in fromMaybe fail $ lookup m ms

lookupMethods :: Type -> ClassTable -> [FunctionHeader]
lookupMethods cls ctable =
    let (_, ms) = lookupEntry cls ctable
    in map snd ms

lookup_implemented_capa :: Type -> ClassTable -> Type
lookup_implemented_capa t ClassTable{program} =
  ccapability $ getClass t program

lookupCalledType :: Type -> Name -> ClassTable -> Type
lookupCalledType ty m ctable
  | isRefType ty = ty
  | isCapabilityType ty =
      let traits = typesFromCapability ty
          ttable = map (\t -> (t, snd $ lookupEntry t ctable)) traits
          results = map (second (lookup m)) ttable
          fail = error $ "ClassTable.hs: No method '" ++ show m ++ "' in " ++
                 Types.showWithKind ty
      in
        fst . fromMaybe fail $ find (isJust . snd) results
