#include <iostream>
#include <fstream>
#include "UniversalTimer.h"
#include "BinaryData.h"
#include "BtcUtils.h"
#include "BlockUtils.h"


using namespace std;

int main(void)
{
   BinaryData bd(32);
   for(int i=0; i<32; i++) 
      bd[i] = i;

   cout << bd.toHexString().c_str() << endl;

   BlockDataManager_FullRAM & bdm = BlockDataManager_FullRAM::GetInstance(); 

   BinaryData genBlock;
   string strgenblk = "0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a29ab5f49ffff001d1dac2b7c";
   genBlock.createFromHex(strgenblk);
   //cout << "The genesis block (hex): " << endl << "\t" << genBlock.toHexString().c_str() << endl;

   BinaryData theHash;   
   for(int i=0; i<50000; i++)
   {
      TIMER_START("BinaryData::GetHash");
      BtcUtils::getHash256(genBlock, theHash);
      TIMER_STOP("BinaryData::GetHash");
   }
   cout << "The hash of the genesis block:" << endl << "\t" << theHash.toHexString().c_str() << endl;
   cout << endl << endl;

   // Reading data from blockchain
   cout << "Reading data from blockchain..." << endl;
   TIMER_START("BDM_Load_and_Scan_BlkChain");
   bdm.readBlkFile_FromScratch("../blk0001.dat");
   TIMER_STOP("BDM_Load_and_Scan_BlkChain");
   cout << endl << endl;

   cout << endl << "Organizing blockchain: " ;
   TIMER_START("BDM_Organize_Chain");
   bool isGenOnMainChain = bdm.organizeChain();
   TIMER_STOP("BDM_Organize_Chain");
   cout << (isGenOnMainChain ? "No Reorg!" : "Reorg Detected!") << endl;
   cout << endl << endl;

   //TIMER_START("BDM_Flag_Transactions");
   //bdm.flagMyTransactions();
   //TIMER_STOP("BDM_Flag_Transactions");


   cout << endl << endl;
   cout << "Printing genesis block information:" << endl;
   bdm.getGenesisBlock().pprint(cout);

   cout << endl << endl;
   cout << "Printing last block information:" << endl;
   bdm.getTopBlockHeader().pprint(cout);

   /////////////////////////////////////////////////////////////////////////////
   cout << endl << endl;
   cout << "Next-to-top block:" << endl;
   BlockHeaderRef & topm1 = *(bdm.getHeaderByHash( bdm.getTopBlockHeader().getPrevHash()));
   topm1.pprint();
   
   /////////////////////////////////////////////////////////////////////////////
   cout << endl << endl;

   cout << endl << endl;
   cout << "Verifying MerkleRoot: ";
   vector<BinaryData> merkletree(0);

   BlockHeaderRef & blk100k = *(bdm.getHeaderByHeight(100014));
   BinaryData merkroot = blk100k.calcMerkleRoot(&merkletree);
   bool isVerified = blk100k.verifyMerkleRoot();
   cout << (isVerified ? "Correct!" : "Incorrect!") 
        << "  ("  << merkroot.toHexString() << ")" << endl;


   cout << endl << endl;
   cout << "Now that we can verify merkle roots, let's verify the " << endl;
   cout << "integrity of the entire blockchain file.  If no bits " << endl;
   cout << "have been flipped, all merkle roots should match the " << endl;
   cout << "headers, and header hashes should have at least four " << endl;
   cout << "leading zeros:" << endl << endl;
   cout << "    Verifying... ";
   TIMER_START("Verify blk0001.dat integrity");
   isVerified = bdm.verifyBlkFileIntegrity();
   TIMER_STOP("Verify blk0001.dat integrity");
   cout << "Done!   Your blkfile " << (isVerified ? "is good!" : " HAS ERRORS") << endl;

   cout << endl << endl;
   uint32_t nTx = blk100k.getNumTx();
   vector<TxRef*> & txrefVect = blk100k.getTxRefPtrList();
   blk100k.pprint();
   cout << "Now print out the txinx/outs for this block:" << endl;
   for(uint32_t t=0; t<nTx; t++)
   {
      TxRef & tx = *txrefVect[t]; 
      uint32_t nIn  = tx.getNumTxIn();
      uint32_t nOut = tx.getNumTxOut();
      for(uint32_t in=0; in<nIn; in++)
      {
         TxInRef txin = tx.getTxInRef(in);
         cout << "TxIn: " << endl;
         TxOutRef prevOut = bdm.getPrevTxOut(txin);
         if(!txin.isCoinbase())
         {
            cout << "\tSender: " << prevOut.getRecipientAddr().toHexString();
            cout << " (" << prevOut.getValue() << ")" << endl;
         }
         else
         {
            cout << "\tSender: " << "<COINBASE/GENERATION>";
            cout << " (50 [probably])" << endl;
         }

      }
      for(uint32_t out=0; out<nOut; out++)
      {
         TxOutRef txout = tx.getTxOutRef(out);
         cout << "TxOut:" << endl;
         cout << "\tRecip: " << txout.getRecipientAddr().toHexString() << endl;
         cout << "\tValue: " << txout.getValue() << endl;
      }
   }


   BinaryData myAddress, myPubKey;
   BtcWallet wlt;
   myAddress.createFromHex("abda0c878dd7b4197daa9622d96704a606d2cd14");
   myPubKey.createFromHex("04e02e7826c63038fa3e6a416b74b85bc4db2b5125f039bb5b0139842655d0faec750ec639c380c0cbc070650037b17a1a6a101391422ff9827a27010990ae1acd");
   wlt.addAddress(myAddress, myPubKey);

   myAddress.createFromHex("11b366edfc0a8b66feebae5c2e25a7b6a5d1cf31");
   wlt.addAddress(myAddress);

   TIMER_WRAP(bdm.scanBlockchainForTx_FromScratch(wlt));
   
   cout << "Checking balance of all addresses: " << wlt.getNumAddr() << " addrs" << endl;
   for(uint32_t i=0; i<wlt.getNumAddr(); i++)
   {
      BinaryData addr20 = wlt.getAddrByIndex(i).getAddrStr20();
      cout << "  Addr: " << wlt.getAddrByIndex(i).getBalance() << ","
                         << wlt.getAddrByHash160(addr20).getBalance() << endl;
      vector<LedgerEntry> const & ledger = wlt.getAddrByIndex(i).getTxLedger();
      for(uint32_t j=0; j<ledger.size(); j++)
      {  
         cout << "    Tx: " 
           << ledger[j].getAddrStr20().getSliceCopy(0,4).toHexString() << "  "
           << ledger[j].getValue()/(float)(CONVERTBTC) << " (" 
           << ledger[j].getBlockNum()
           << ")  TxHash: " << ledger[j].getTxHash().getSliceCopy(0,4).toHexString() << endl;
      }

   }
   cout << endl;

   myAddress.createFromHex("f62242a747ec1cb02afd56aac978faf05b90462e");
   wlt.addAddress(myAddress);
   TIMER_WRAP(bdm.scanBlockchainForTx_FromScratch(wlt));

   cout << "Checking balance of all addresses: " << wlt.getNumAddr() << "addrs" << endl;
   for(uint32_t i=0; i<wlt.getNumAddr(); i++)
   {
      BinaryData addr20 = wlt.getAddrByIndex(i).getAddrStr20();
      cout << "  Addr: " << wlt.getAddrByIndex(i).getBalance() << ","
                         << wlt.getAddrByHash160(addr20).getBalance() << endl;

   }

   cout << "Printing unsorted allAddr ledger..." << endl;
   wlt.cleanLedger();
   vector<LedgerEntry> const & ledger = wlt.getTxLedger();
   for(uint32_t j=0; j<ledger.size(); j++)
   {  
      cout << "    Tx: " 
           << ledger[j].getAddrStr20().toHexString() << "  "
           << ledger[j].getValue()/(float)(CONVERTBTC) << " (" 
           << ledger[j].getBlockNum()
           << ")  TxHash: " << ledger[j].getTxHash().getSliceCopy(0,4).toHexString() << endl;
   }

   cout << "Printing SORTED allAddr ledger..." << endl;
   vector<LedgerEntry> const & ledger2 = wlt.getTxLedger();
   for(uint32_t j=0; j<ledger2.size(); j++)
   {  
      cout << "    Tx: " 
           << ledger2[j].getAddrStr20().toHexString() << "  "
           << ledger2[j].getValue()/(float)(CONVERTBTC) << " (" 
           << ledger2[j].getBlockNum()
           << ")  TxHash: " << ledger2[j].getTxHash().getSliceCopy(0,4).toHexString() << endl;
           
   }

   cout << endl << endl;
   cout << "Scanning the blockchain for all non-std transactions..." << endl;
   TIMER_START("FindNonStdTx");
   vector<TxRef*> nonStdTxVect = bdm.findAllNonStdTx();
   TIMER_STOP("FindNonStdTx");
   cout << endl << "Found " << nonStdTxVect.size() << " such transactions:" << endl;
   for(uint32_t i=0; i<nonStdTxVect.size(); i++)
   {
      TxRef & tx = *(nonStdTxVect[i]);
      cout << "  Block:  " << tx.getHeaderPtr()->getBlockHeight() << endl;
      cout << "  TxHash: " << tx.getThisHash().copySwapEndian().toHexString() << endl;
      cout << "  #TxIn:  " << tx.getNumTxIn() << endl;
      cout << "  #TxOut: " << tx.getNumTxOut() << endl;
      cout << endl << endl;
   }

   // Last I checked, resetting doesn't actually work!
   TIMER_WRAP(bdm.Reset());


   // ***** Print out all timings to stdout and a csv file *****
   //       This file can be loaded into a spreadsheet,
   //       but it's not the prettiest thing...
   UniversalTimer::instance().print();
   UniversalTimer::instance().printCSV("timings.csv");


   char aa[256];
   cout << "Enter anything to exit" << endl;
   cin >> aa;
}
