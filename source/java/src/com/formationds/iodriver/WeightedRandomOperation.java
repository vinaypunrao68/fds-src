//package com.formationds.iodriver;
//
//import java.util.Collections;
//import java.util.Map;
//import java.util.Map.Entry;
//import java.util.NavigableMap;
//import java.util.Random;
//import java.util.TreeMap;
//
//import com.formationds.iodriver.endpoints.Endpoint;
//import com.formationds.iodriver.operations.Operation;
//
//public final class WeightedRandomOperation extends Operation
//{
//    public WeightedRandomOperation(Random randomSource)
//    {
//        this(Collections.emptyMap(), randomSource);
//    }
//    
//    public WeightedRandomOperation(Map<Operation, Integer> operations, Random randomSource)
//    {
//        if (operations == null) throw new NullArgumentException("operations");
//        if (randomSource == null) throw new NullArgumentException("randomSource");
//        
//        NavigableMap<Integer, Operation> validatedOperations = new TreeMap<>();
//        int newTotalWeight = 0;
//        for (Entry<Operation, Integer> entry : operations.entrySet())
//        {
//            Integer weight = entry.getValue();
//            if (weight < 0)
//            {
//                throw new IllegalArgumentException("operations cannot have a negative weight: " + entry.getKey() + " has weight " + weight + ".");
//            }
//            
//            validatedOperations.put(newTotalWeight, entry.getKey());
//            newTotalWeight += weight;
//        }
//        
//        _operations = validatedOperations;
//        _randomSource = randomSource;
//        _totalWeight = newTotalWeight;
//    }
//    
//    @Override
//    public void runOn(Endpoint endpoint)
//    {
//        if (endpoint == null) throw new NullArgumentException("endpoint");
//        
//        if (!_operations.isEmpty())
//        {
//            int selection = _randomSource.nextInt(_totalWeight);
//            Operation operation = _operations.floorEntry(selection).getValue();
//            
//            operation.runOn(endpoint);
//        }
//    }
//    
//    private final NavigableMap<Integer, Operation> _operations;
//    
//    private final Random _randomSource;
//    
//    private final int _totalWeight;
//}
